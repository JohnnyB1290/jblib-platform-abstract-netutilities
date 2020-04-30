/*
 * The MIT License (MIT)
 *
 * Copyright © 2015 by Sergey Fetisov <fsenok@gmail.com>
 * Copyright © 2020 Evgeniy Ivanov. Contacts: <strelok1290@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * version: 1.0 demo (7.02.2015)
 * brief:   tiny dns ipv4 server using lwip (pcb)
 */

// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "jbkernel/jb_common.h"
#if USE_LWIP && JB_LIB_OS != 0
#include <cstring>
#include <sys/socket.h>
#include <tcpip_adapter.h>
#include "ethutilities/freertos/DnsServer.hpp"
#include "jbdrivers/JbController.hpp"

namespace jblib
{
namespace ethutilities
{

using namespace ::jblib::jbkernel;
using namespace ::jblib::jbdrivers;


#pragma pack(push, 1)
typedef struct
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t rd: 1,     /* Recursion Desired */
            tc: 1,     /* Truncation Flag */
            aa: 1,     /* Authoritative Answer Flag */
            opcode: 4, /* Operation code */
            qr: 1;     /* Query/Response Flag */
    uint8_t rcode: 4,  /* Response Code */
            z: 3,      /* Zero */
            ra: 1;     /* Recursion Available */
#else
    uint8_t qr: 1,     /* Query/Response Flag */
            opcode: 4, /* Operation code */
            aa: 1,     /* Authoritative Answer Flag */
            tc: 1,     /* Truncation Flag */
            rd: 1;     /* Recursion Desired */
    uint8_t ra: 1,     /* Recursion Available */
            z: 3,      /* Zero */
            rcode: 4;  /* Response Code */
#endif
} dns_header_flags_t;



typedef struct
{
    uint16_t id;
    dns_header_flags_t flags;
    uint16_t n_record[4];
} dns_header_t;



typedef struct dns_answer
{
    uint16_t name;
    uint16_t type;
    uint16_t Class;
    uint32_t ttl;
    uint16_t len;
    uint32_t addr;
} dns_answer_t;

#pragma pack(pop)

DnsServer* DnsServer::dnsServer_ = nullptr;


DnsServer* DnsServer::getDnsServer()
{
    if(dnsServer_ == nullptr){
        dnsServer_ = new DnsServer();
    }
    return dnsServer_;
}



DnsServer::DnsServer() : IVoidCallback()
{
    #if !CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE && (JB_LIB_PLATFORM == 3)
    esp_log_level_set(logTag_, ESP_LOG_WARN);
    #endif
}



void DnsServer::addHost(char* hostName)
{
    auto* host = (DnsHost_t*)malloc_s(sizeof(DnsHost_t));
    if(host){
        strncpy(host->name, hostName, CONFIG_JBLIB_DNS_SERVER_HOST_NAME_MAX_SIZE);
        xSemaphoreTake(this->hostsListMutex_, portMAX_DELAY);
        this->hostsList_.push_front(*host);
        xSemaphoreGive(this->hostsListMutex_);
        free_s(host);
    }
}



void DnsServer::deleteHost(char* hostName)
{
    xSemaphoreTake(this->hostsListMutex_, portMAX_DELAY);
    if(!this->hostsList_.empty()){
        this->hostsList_.remove_if([hostName](DnsHost_t item){
             return !strncmp(hostName, item.name,
                     CONFIG_JBLIB_DNS_SERVER_HOST_NAME_MAX_SIZE);
        });
    }
    xSemaphoreGive(this->hostsListMutex_);
}



void DnsServer::start()
{
    if(!this->isStarted_){
        #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
        #if JB_LIB_PLATFORM == 3
        ESP_LOGI(logTag_, "Start DNS Server");
        #else
        #if USE_CONSOLE
        printf("%s Start DNS Server\n", logTag_);
        #endif
        #endif
        #endif
        tcpip_adapter_ip_info_t ipInfo;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo);
        this->replyIp_= ipInfo.ip.addr;
        this->socket_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (this->socket_ == -1) {
            #if JB_LIB_PLATFORM == 3
            ESP_LOGE(logTag_, "Can't create socket");
            perror("Failed to create socket");
            #else
            #if USE_CONSOLE
            printf("%s Failed to create socket\n", logTag_);
            #endif
            #endif
            return;
        }
        struct sockaddr_in sin{};
        memset(&sin,0, sizeof(sin));
        sin.sin_family = PF_INET;
        sin.sin_port = htons(CONFIG_JBLIB_DNS_SERVER_PORT);
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        int err = bind(this->socket_, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
        if (err < 0){
            #if JB_LIB_PLATFORM == 3
            ESP_LOGE(logTag_, "Can't bind socket");
            perror("Failed to bind socket");
            #else
            #if USE_CONSOLE
            printf("%s Failed to bind socket\n", logTag_);
            #endif
            #endif
            close(socket_);
            socket_ = -1;
            return;
        }
        JbController::addMainProcedure(this, nullptr,
                CONFIG_JBLIB_DNS_SERVER_THREAD_STACK_SIZE,
                CONFIG_JBLIB_DNS_SERVER_THREAD_PRIORITY,
                "DnsServer");
        this->isStarted_ = true;
    }
    #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
    else{
        #if JB_LIB_PLATFORM == 3
        ESP_LOGI(logTag_, "Has been started already!");
        #else
        #if USE_CONSOLE
        printf("%s Has been started already!\n", logTag_);
        #endif
        #endif
    }
    #endif
}



void DnsServer::stop()
{
    if(this->isStarted_){
        #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
        #if JB_LIB_PLATFORM == 3
        ESP_LOGI(logTag_, "Stop DNS Server");
        #else
        #if USE_CONSOLE
        printf("%s Stop DNS Server\n", logTag_);
        #endif
        #endif
        #endif
        JbController::deleteMainProcedure(this);
        closesocket(this->socket_);
        this->socket_ = -1;
        isStarted_ = false;
    }
    #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
    else{
        #if JB_LIB_PLATFORM == 3
        ESP_LOGI(logTag_, "Hasn't been started yet!");
        #else
        #if USE_CONSOLE
        printf("%s Hasn't been started yet!\n", logTag_);
        #endif
        #endif
    }
    #endif
}



void DnsServer::voidCallback(void* const source, void* parameter)
{
    static dns_query_t query;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(this->socket_, &rfds);
    int s = select(this->socket_ + 1, &rfds, nullptr, nullptr, nullptr);
    if (s < 0) {
        #if JB_LIB_PLATFORM == 3
        ESP_LOGE(logTag_, "Select error, stop DNS server");
        perror("Select error");
        #else
        #if USE_CONSOLE
            printf("%s Select error, stop DNS server\n", logTag_);
            #endif
        #endif
        this->stop();
        return;
    }
    else if ((s > 0) && FD_ISSET(this->socket_, &rfds)){
        uint8_t recvBuffer[CONFIG_JBLIB_DNS_SERVER_RECIEVE_BUFFER_SIZE];
        struct sockaddr_in remotehost{};
        socklen_t socklen = sizeof(remotehost);
        int recLen = recvfrom(this->socket_, recvBuffer,
                CONFIG_JBLIB_DNS_SERVER_RECIEVE_BUFFER_SIZE, 0,
                (struct sockaddr*)&remotehost, &socklen);
        if(recLen < 0){
            #if JB_LIB_PLATFORM == 3
            ESP_LOGE(logTag_, "Recvfrom error, stop DNS server");
            perror("Recvfrom error");
            #else
            #if USE_CONSOLE
            printf("%s Recvfrom error, stop DNS server\n", logTag_);
            #endif
            #endif
            this->stop();
            return;
        }
        #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
        #if JB_LIB_PLATFORM == 3
        ESP_LOGI(logTag_, "Recieve packet");
        #else
        #if USE_CONSOLE
        printf("%s Recieve packet\n", logTag_);
        #endif
        #endif
        #endif
        if (recLen <= sizeof(dns_header_t)){
            #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
            #if JB_LIB_PLATFORM == 3
            ESP_LOGW(logTag_, "Packet too small");
            #else
            #if USE_CONSOLE
            printf("%s Packet too small\n", logTag_);
            #endif
            #endif
            #endif
            return;
        }
        auto* header = (dns_header_t*)recvBuffer;
        if (header->flags.qr != 0){
            #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
            #if JB_LIB_PLATFORM == 3
            ESP_LOGW(logTag_, "Packet isn't a query");
            #else
            #if USE_CONSOLE
            printf("%s Packet isn't a query\n", logTag_);
            #endif
            #endif
            #endif
            return;
        }
        if(ntohs(header->n_record[0]) != 1){
            #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
            #if JB_LIB_PLATFORM == 3
            ESP_LOGW(logTag_, "n_record[0] != 1");
            #else
            #if USE_CONSOLE
            printf("%s n_record[0] != 1\n", logTag_);
            #endif
            #endif
            #endif
            return;
        }
        int len = parseNextQuery(header + 1,
                recLen - sizeof(dns_header_t), &query);
        if (len < 0){
            #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
            #if JB_LIB_PLATFORM == 3
            ESP_LOGW(logTag_, "Can't parse Next Query");
            #else
            #if USE_CONSOLE
            printf("%s Can't parse Next Query\n", logTag_);
            #endif
            #endif
            #endif
            return;
        }
        #if !CONFIG_JBLIB_DNS_SERVER_RESPONSE_TO_ALL_REQUESTS
        bool requestForMe = false;
        xSemaphoreTake(this->hostsListMutex_, portMAX_DELAY);
        if(!this->hostsList_.empty()){
            for(auto host : this->hostsList_){
                if(!strcmp(query.name, host.name)){
                    requestForMe = true;
                }
            }
        }
        xSemaphoreGive(this->hostsListMutex_);
        if(!requestForMe){
            #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
            #if JB_LIB_PLATFORM == 3
            ESP_LOGI(logTag_, "Request not to my host");
            #else
            #if USE_CONSOLE
            printf("%s Request not to my host\n", logTag_);
            #endif
            #endif
            #endif
            return;
        }
        #endif
        len += sizeof(dns_header_t);
        header = (dns_header_t *)recvBuffer;
        header->flags.qr = 1;
        header->n_record[1] = htons(1);
        auto* answer = (struct dns_answer*)(recvBuffer + len);
        answer->name = htons(0xC00C);
        answer->type = htons(1);
        answer->Class = htons(1);
        answer->ttl = htonl(32);
        answer->len = htons(4);
        answer->addr = this->replyIp_;
        sendto(this->socket_, recvBuffer, len + sizeof(struct dns_answer),
               0, (struct sockaddr*)&remotehost, socklen);
        #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
        #if JB_LIB_PLATFORM == 3
        ESP_LOGI(logTag_, "Send answer");
        #else
        #if USE_CONSOLE
        printf("%s Send answer\n", logTag_);
        #endif
        #endif
        #endif
    }
}



int DnsServer::parseNextQuery(void *data, int size, dns_query_t *query)
{
    int len = 0;
    int lables = 0;
    auto* ptr = (uint8_t *)data;
    while (true) {
        uint8_t lable_len;
        if (size <= 0) return -1;
        lable_len = *ptr++;
        size--;
        if (lable_len == 0) break;
        if (lables > 0)
        {
            if (len == CONFIG_JBLIB_DNS_SERVER_HOST_NAME_MAX_SIZE){
                return -2;
            }
            query->name[len++] = '.';
        }
        if (lable_len > size) return -1;
        if (len + lable_len >= CONFIG_JBLIB_DNS_SERVER_HOST_NAME_MAX_SIZE){
            return -2;
        }
        memcpy(&query->name[len], ptr, lable_len);
        len += lable_len;
        ptr += lable_len;
        size -= lable_len;
        lables++;
    }
    if (size < 4) return -1;
    query->name[len] = 0;

    #if CONFIG_JBLIB_DNS_SERVER_CONSOLE_ENABLE
    #if JB_LIB_PLATFORM == 3
    ESP_LOGI(logTag_, "Request for:%s", query->name);
    #else
    #if USE_CONSOLE
    printf("%s Request for:%s\n", logTag_, query->name);
    #endif
    #endif
    #endif
    memcpy(&query->type,ptr,2);
    ptr += 2;
    memcpy(&query->Class,ptr,2);
    ptr += 2;
    return ptr - (uint8_t *)data;
}

}
}

#endif
