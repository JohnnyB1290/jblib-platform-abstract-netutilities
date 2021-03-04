/**
 * @file
 * @brief UDP void channel class Realization
 *
 *
 * @note
 * Copyright © 2021 Evgeniy Ivanov. Contacts: <strelok1290@gmail.com>
 * All rights reserved.
 * @note
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * @note
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @note
 * This file is a part of JB_Lib.
 */

// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <lwip/ip_addr.h>
#include "jbkernel/jb_common.h"
#if USE_LWIP && JB_LIB_OS != 0
#include "ethutilities/freertos/UdpVoidChannel.hpp"
#include <esp_pthread.h>
#include <thread>
#include <vector>

namespace jblib
{
    namespace ethutilities
    {
        using namespace jbkernel;

        UdpVoidChannel::UdpVoidChannel(uint16_t srcPort) : VoidChannel()
        {
            this->construct(ip_addr_any, srcPort);
        }



        UdpVoidChannel::UdpVoidChannel(ip_addr_t srcIp, uint16_t srcPort) : VoidChannel()
        {
            this->construct(srcIp, srcPort);
        }



        void UdpVoidChannel::construct(ip_addr_t srcIp, uint16_t srcPort)
        {
            #if !CONFIG_JBLIB_UDP_CHANNEL_CONSOLE_ENABLE && (JB_LIB_PLATFORM == 3)
            esp_log_level_set(logTag_, ESP_LOG_WARN);
            #endif
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
            struct sockaddr_in srcAddr{};
            srcAddr.sin_family = PF_INET;
            srcAddr.sin_addr.s_addr = srcIp.u_addr.ip4.addr;
            srcAddr.sin_port = htons(srcPort);
            int err = bind(this->socket_, reinterpret_cast<const sockaddr *>(&srcAddr), sizeof(struct sockaddr_in));
            if (err < 0){
                #if JB_LIB_PLATFORM == 3
                ESP_LOGE(logTag_, "Can't bind socket");
                perror("Failed to bind socket");
                #else
                #if USE_CONSOLE
                printf("%s Failed to bind socket\n", logTag_);
                #endif
                #endif
                closesocket(this->socket_);
                this->socket_ = -1;
                return;
            }

            auto cfg = esp_pthread_get_default_config();
            std::function<void()> rxHandlerFunc = std::bind( &UdpVoidChannel::rxHandler, this);
            cfg.thread_name = logTag_;
            cfg.stack_size = CONFIG_JBLIB_UDP_CHANNEL_THREAD_STACK_SIZE;
            cfg.prio = CONFIG_JBLIB_UDP_CHANNEL_THREAD_PRIORITY;
            esp_pthread_set_cfg(&cfg);
            std::thread handlerThread(rxHandlerFunc);
            handlerThread.detach();
        }



        void UdpVoidChannel::rxHandler()
        {
            auto errorExit = [this](){
                #if JB_LIB_PLATFORM == 3
                ESP_LOGE(logTag_, "Close socket because of error");
                perror("Close socket");
                #else
                #if USE_CONSOLE
                printf("%s Close socket because of error\n", logTag_);
                #endif
                #endif
                closesocket(this->socket_);
                this->socket_ = -1;
            };
            struct pollfd fds = {
                    .fd = this->socket_,
                    .events = POLLIN,
                    .revents = 0
            };
            while(true){
                if(this->socket_ < 0){
                    return;
                }
                fds.revents = 0;
                int result = poll(&fds, 1, -1);
                if((result > 0) && (fds.revents & POLLIN)){
                    int count = 0;
                    result = ioctlsocket(this->socket_, FIONREAD, &count);
                    if(result >= 0){
                        std::vector<uint8_t> data(count);
                        struct sockaddr_in srcAddr{};
                        socklen_t socklen = sizeof(srcAddr);
                        result = recvfrom(this->socket_, data.data(), count, 0,
                                reinterpret_cast<sockaddr *>(&srcAddr), &socklen);
                        if(result > 0){
                            ESP_LOGI(logTag_, "Received %i bytes from %s:%i", result,
                                    inet_ntoa(srcAddr.sin_addr), htons(srcAddr.sin_port));
                            if(this->callback_){
                                this->callback_(data.data(), result, this, &srcAddr);
                            }
                            std::lock_guard<std::mutex> lockGuard(this->dstAddrMutex_);
                            this->dstAddr_ = srcAddr;
                        }
                        else if(result < 0){
                            errorExit();
                            return;
                        }
                    }
                    else{
                        errorExit();
                        return;
                    }
                }
                else if(result < 0){
                    errorExit();
                    return;
                }
            }
        }



        UdpVoidChannel::~UdpVoidChannel()
        {
            if(this->socket_ > 0){
                closesocket(this->socket_);
            }
        }



        void UdpVoidChannel::tx(uint8_t* data, uint16_t size, void* connectionParameter)
        {
            if(this->socket_ < 0){
                return;
            }
            struct sockaddr_in dstAddr{};
            if(connectionParameter){
                dstAddr = *reinterpret_cast<struct sockaddr_in*>(connectionParameter);
            }
            else{
                std::lock_guard<std::mutex> lockGuard(this->dstAddrMutex_);
                dstAddr = this->dstAddr_;
            }
            int result = sendto(this->socket_, data, size, O_NONBLOCK | MSG_DONTWAIT,
                    reinterpret_cast<const sockaddr *>(&dstAddr), sizeof(dstAddr));
            if(result >= 0){
                ESP_LOGI(logTag_, "Sent %i bytes to %s:%i", result,
                        inet_ntoa(dstAddr.sin_addr), htons(dstAddr.sin_port));
            } else if(errno != 118){ //118 error = net is not connected
                closesocket(this->socket_);
                this->socket_ = -1;
            }
        }

    }
}

#endif