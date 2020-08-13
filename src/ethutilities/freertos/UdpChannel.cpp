/**
 * @file
 * @brief UDP void channel class Realization
 *
 *
 * @note
 * Copyright © 2020 Evgeniy Ivanov. Contacts: <strelok1290@gmail.com>
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

#include "jbkernel/jb_common.h"
#if USE_LWIP && JB_LIB_OS != 0

#include <cstring>
#include "ethutilities/freertos/UdpChannel.hpp"
#include "jbdrivers/JbController.hpp"

namespace jblib
{
    namespace ethutilities
    {
        using namespace jbkernel;

        UdpChannel::UdpChannel(uint16_t srcPort, uint8_t* dstIp, uint16_t dstPort) : IVoidChannel(), IVoidCallback()
        {
            this->construct(nullptr, srcPort, dstIp, dstPort);
        }



        UdpChannel::UdpChannel(uint8_t* srcIp,
                uint16_t srcPort, uint8_t* dstIp, uint16_t dstPort) : IVoidChannel(), IVoidCallback()
        {
            this->construct(srcIp, srcPort, dstIp, dstPort);
        }



        void UdpChannel::construct(const uint8_t* srcIp, uint16_t srcPort, const uint8_t* dstIp, uint16_t dstPort)
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
            memset(&this->srcAddr_, 0, sizeof(this->srcAddr_));
            this->srcAddr_.sin_family = PF_INET;
            this->srcAddr_.sin_port = htons(srcPort);
            if(srcIp){
                this->srcAddr_.sin_addr.s_addr =
                        htonl(LWIP_MAKEU32(srcIp[0], srcIp[1], srcIp[2], srcIp[3]));
            }
            else{
                this->srcAddr_.sin_addr.s_addr = htonl(INADDR_ANY);
            }
            int err = bind(this->socket_, (struct sockaddr *)&this->srcAddr_, sizeof(struct sockaddr_in));
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
            memset(&this->dstAddr_, 0, sizeof(this->dstAddr_));
            this->dstAddr_.sin_family = PF_INET;
            this->dstAddr_.sin_port = htons(srcPort);
            if(dstIp){
                this->dstAddr_.sin_addr.s_addr =
                        htonl(LWIP_MAKEU32(dstIp[0], dstIp[1], dstIp[2], dstIp[3]));
            }
            else{
                this->dstAddr_.sin_addr.s_addr = htonl(INADDR_ANY);
            }
            this->dstAddr_.sin_port = htons(dstPort);
            this->receiveTaskHandle_ = JbKernel::addMainProcedure(this, nullptr,
                    CONFIG_JBLIB_UDP_CHANNEL_THREAD_STACK_SIZE,
                    CONFIG_JBLIB_UDP_CHANNEL_THREAD_PRIORITY, logTag_);
        }



        UdpChannel::~UdpChannel()
        {
            if((this->socket_ < 0)){
                closesocket(this->socket_);
            }
            if(this->receiveTaskHandle_){
                vTaskDelete(this->receiveTaskHandle_->taskHandle);
                free_s(this->receiveTaskHandle_);
            }
        }



        void UdpChannel::tx(uint8_t* const buffer, const uint16_t size, void* parameter)
        {
            struct sockaddr_in dstAddr{};
            memset(&dstAddr, 0, sizeof(sockaddr_in));
            dstAddr.sin_family = PF_INET;
            auto* connParam = (IVoidChannel::ConnectionParameter_t*)parameter;
            if(connParam == nullptr){
                memcpy(&dstAddr, &this->dstAddr_, sizeof(sockaddr_in));
            }
            else{
                auto dstHost = (UdpHost_t*)(connParam->parameters);
                dstAddr.sin_port = htons(dstHost->port);
                dstAddr.sin_addr.s_addr = dstHost->host;
            }
            int length = sendto(this->socket_, buffer, size, O_NONBLOCK | MSG_DONTWAIT, (struct sockaddr*)&dstAddr, sizeof(dstAddr));
            ESP_LOGI(logTag_, "sendto %i bytes", length);
            if (length < 0) {
                #if !JBLIB_UDP_CHANNEL_CONSOLE_ENABLE
                if(errno == 118){
                    return;
                }
                #endif
                #if JB_LIB_PLATFORM == 3
                ESP_LOGI(logTag_, "sendto failed: errno %d", errno);
                #else
                #if USE_CONSOLE
                printf("%s recvfrom failed: errno %d\n", logTag_, errno);
                #endif
                #endif
                return;
            }
        }



        void UdpChannel::setDestination(UdpHost_t* to)
        {
            this->dstAddr_.sin_port = htons(to->port);
            this->dstAddr_.sin_addr.s_addr = to->host;
        }



        uint32_t UdpChannel::getDestinationHost()
        {
            return this->dstAddr_.sin_addr.s_addr;
        }



        uint16_t UdpChannel::getDestinationPort()
        {
            return this->dstAddr_.sin_port;
        }



        void UdpChannel::initialize(void* (* const mallocFunc)(size_t),
                                    const uint16_t txBufferSize, IChannelCallback* const callback)
        {
            this->callback_ = callback;
        }



        void UdpChannel::deinitialize()
        {
            this->callback_ = nullptr;
        }



        void UdpChannel::getParameter(const uint8_t number, void* const value)
        {

        }



        void UdpChannel::setParameter(const uint8_t number, void* const value)
        {

        }



        void UdpChannel::voidCallback(void* source, void* parameter)
        {
            struct sockaddr_in srcAddr{};
            memset(&srcAddr, 0, sizeof(sockaddr_in));
            socklen_t socklen = sizeof(srcAddr);
            int length = recvfrom(this->socket_, this->rxBuffer_, sizeof(this->rxBuffer_),
                    0, (struct sockaddr *)&srcAddr, &socklen);
            if (length < 0) {
                #if JB_LIB_PLATFORM == 3
                ESP_LOGE(logTag_, "recvfrom failed: errno %d", errno);
                #else
                #if USE_CONSOLE
                printf("%s recvfrom failed: errno %d\n", logTag_, errno);
                #endif
                #endif
                return;
            }
            else {
                this->dmSource_.port = ntohs(srcAddr.sin_port);
                this->dmSource_.host = srcAddr.sin_addr.s_addr;
                IVoidChannel::ConnectionParameter_t connParam;
                connParam.parameters = &(this->dmSource_);
                connParam.parametersSize = sizeof(UdpHost_t);
                if(this->callback_){
                    this->callback_->channelCallback(this->rxBuffer_,
                            length, this, &connParam);
                }
            }
        }
    }
}

#endif