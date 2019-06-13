/**
 * @file
 * @brief UDP void channel class Description
 *
 *
 * @note
 * Copyright © 2019 Evgeniy Ivanov. Contacts: <strelok1290@gmail.com>
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

#ifndef UDP_CHANNEL_HPP_
#define UDP_CHANNEL_HPP_

#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "jbkernel/IVoidChannel.hpp"

namespace jblib
{
namespace ethutilities
{

using namespace jbkernel;

class UdpChannel : public IVoidChannel
{
public:
	UdpChannel(uint8_t* srcIp, uint16_t srcPort,
			uint8_t* dstIp, uint16_t dstPort, struct netif* netif);
	UdpChannel(uint16_t srcPort,
			uint8_t* dstIp, uint16_t dstPort, struct netif* netif);
	UdpChannel(uint16_t srcPort, uint8_t* dstIp, uint16_t dstPort);
	virtual ~UdpChannel(void);
	virtual void initialize(void* (* const mallocFunc)(size_t),
			const uint16_t txBufferSize, IChannelCallback* const callback);
	virtual void deinitialize(void);
	virtual void tx(uint8_t* const buffer, const uint16_t size, void* parameter);
	virtual void getParameter(const uint8_t number, void* const value);
	virtual void setParameter(const uint8_t number, void* const value);

private:
	static void recieveCallback(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port);
	void construct(uint8_t* srcIp, uint16_t srcPort,
			uint8_t* dstIp, uint16_t dstPort, struct netif* netif);

	struct udp_pcb* pcb_ = NULL;
	ip_addr_t srcIpaddr_ = {.addr = 0};
	uint16_t srcPort_ = 0;
	ip_addr_t dstIpaddr_ = {.addr = 0};
	uint16_t dstPort_ = 0;
	struct netif* netif_ = NULL;
	IChannelCallback* callback_ = NULL;
	struct pbuf* pTx_ = NULL;
};

}
}

#endif /* UDP_CHANNEL_HPP_ */
