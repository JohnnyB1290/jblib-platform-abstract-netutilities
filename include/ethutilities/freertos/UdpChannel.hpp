/**
 * @file
 * @brief UDP void channel class Description
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

#ifndef UDPCHANNEL_HPP
#define UDPCHANNEL_HPP

#include "jbkernel/IVoidChannel.hpp"
#include "jbkernel/JbKernel.hpp"
#include <sys/socket.h>

namespace jblib
{
    namespace ethutilities
    {

        using namespace jbkernel;

        #pragma pack(push, 1)
        typedef struct
        {
            uint32_t host = 0;
            uint16_t port = 0;
        } UdpHost_t;
        #pragma pack(pop)


        class UdpChannel : public IVoidChannel, IVoidCallback
        {
        public:
            UdpChannel(uint8_t* srcIp, uint16_t srcPort, uint8_t* dstIp, uint16_t dstPort);
            UdpChannel(uint16_t srcPort, uint8_t* dstIp, uint16_t dstPort);
            virtual ~UdpChannel(void);
            virtual void initialize(void* (* const mallocFunc)(size_t),
                                    const uint16_t txBufferSize, IChannelCallback* const callback);
            virtual void deinitialize(void);
            virtual void tx(uint8_t* const buffer, const uint16_t size, void* parameter);
            virtual void getParameter(const uint8_t number, void* const value);
            virtual void setParameter(const uint8_t number, void* const value);
            void setDestination(UdpHost_t* to);
            uint32_t getDestinationHost();
            uint16_t getDestinationPort();

        protected:
            virtual void voidCallback(void* source, void* parameter);

            int socket_ = -1;
            struct sockaddr_in srcAddr_{};
            struct sockaddr_in dstAddr_{};
            uint8_t rxBuffer_[CONFIG_JBLIB_UDP_CHANNEL_RX_BUFFER_SIZE]{};
            UdpHost_t dmSource_;
            IChannelCallback* callback_ = nullptr;

        private:
            static constexpr const char* logTag_ = "[ UDP Channel ]";
            void construct(uint8_t* srcIp, uint16_t srcPort, uint8_t* dstIp, uint16_t dstPort);

            jblib::jbkernel::JbKernel::ProceduresListItem* receiveTaskHandle_ = nullptr;
        };
    }
}


#endif //UDPCHANNEL_HPP
