/**
 * @file
 * @brief UDP void channel class Description
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

#pragma once

#include "jbkernel/IVoidChannel.hpp"
#include "jbkernel/JbKernel.hpp"
#include <sys/socket.h>
#include <mutex>

namespace jblib
{
    namespace ethutilities
    {
        using namespace jbkernel;

        class UdpVoidChannel : public ::jblib::jbkernel::VoidChannel
        {
            static constexpr const char* logTag_ = "[ UDP Void Channel ]";
            void construct(ip_addr_t srcIp, uint16_t srcPort);
            void rxHandler();

        protected:
            int socket_ = -1;
            std::mutex dstAddrMutex_;
            struct sockaddr_in dstAddr_{};

        public:
            UdpVoidChannel(ip_addr_t srcIp, uint16_t srcPort);
            explicit UdpVoidChannel(uint16_t srcPort);
            ~UdpVoidChannel() override;
            void initialize() override {};
            //connectionParameter == &sockaddr_in
            void tx(uint8_t* data, uint16_t size, void* connectionParameter) override;
        };
    }
}
