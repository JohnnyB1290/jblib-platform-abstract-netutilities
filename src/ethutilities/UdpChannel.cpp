/**
 * @file
 * @brief UDP void channel class Realization
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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "jbkernel/jb_common.h"
#if USE_LWIP && (JB_LIB_OS == 0)
#include <string.h>
#include "lwip/udp.h"
#include "lwip/igmp.h"
#include "ethutilities/UdpChannel.hpp"
#include "jbkernel/jb_common.h"

namespace jblib::ethutilities
{

using namespace jbkernel;



UdpChannel::UdpChannel(uint16_t srcPort, uint8_t* dstIp,
		uint16_t dstPort, struct netif* netif) : IVoidChannel()
{
	this->construct(NULL, srcPort, dstIp, dstPort, netif);
}



UdpChannel::UdpChannel(uint16_t srcPort,
		uint8_t* dstIp, uint16_t dstPort) : IVoidChannel()
{
	this->construct(NULL, srcPort, dstIp, dstPort, NULL);
}



UdpChannel::UdpChannel(uint8_t* srcIp, uint16_t srcPort,
		uint8_t* dstIp, uint16_t dstPort, struct netif* netif) : IVoidChannel()
{
	this->construct(srcIp, srcPort, dstIp, dstPort, netif);
}



void UdpChannel::construct(uint8_t* srcIp, uint16_t srcPort,
		uint8_t* dstIp, uint16_t dstPort, struct netif* netif)
{
	if(srcIp)
		IP4_ADDR(&this->srcIpaddr_, srcIp[0], srcIp[1], srcIp[2], srcIp[3]);
	else
		this->srcIpaddr_ = ip_addr_any;
	this->srcPort_ = srcPort;
	if(dstIp)
		IP4_ADDR(&this->dstIpaddr_, dstIp[0], dstIp[1], dstIp[2], dstIp[3]);
	else
		this->dstIpaddr_ = ip_addr_any;
	this->dstPort_ = dstPort;
	this->netif_ = netif;
#if LWIP_IGMP == 1
	if(ip4_addr_ismulticast(&this->dstIpaddr_))
		igmp_joingroup(&this->srcIpaddr_,&this->dstIpaddr_);
#endif
	udp_init();
	this->pcb_ = udp_new();
	udp_recv(this->pcb_, recieveCallback, this);
	udp_bind(this->pcb_, &this->srcIpaddr_, this->srcPort_);
}



UdpChannel::~UdpChannel(void)
{
	udp_remove(this->pcb_);
}



void UdpChannel::tx(uint8_t* const buffer, const uint16_t size, void* parameter)
{
	uint32_t unbufSize = size;
	uint32_t payloadSize = 0;
	uint8_t* data = buffer;
	ip_addr_t dstIpaddr = {.addr = 0};
	uint16_t dstPort = 0;
	IVoidChannel::ConnectionParameter_t* connParam =
			(IVoidChannel::ConnectionParameter_t*)parameter;
	if(connParam == NULL){
		dstIpaddr = this->dstIpaddr_;
		dstPort = this->dstPort_;
	}
	else{
		UdpHost_t* dstHost = (UdpHost_t*)(connParam->parameters);
		ip_addr_set_ip4_u32(&dstIpaddr, dstHost->host);
		dstPort = dstHost->port;
	}
	while(unbufSize>0) {
		payloadSize = MIN(unbufSize, TCP_MSS);
		this->pTx_ = pbuf_alloc(PBUF_TRANSPORT, payloadSize, PBUF_RAM);
		if (this->pTx_ == NULL)
			break;
		memcpy(this->pTx_->payload, data, payloadSize);
		data = data + payloadSize*sizeof(uint8_t);
		if(this->netif_ == (netif*)NULL)
			udp_sendto(this->pcb_, this->pTx_, &dstIpaddr, dstPort);
		else
			udp_sendto_if(this->pcb_, this->pTx_, &dstIpaddr,
					dstPort, this->netif_);
		pbuf_free(this->pTx_);
		unbufSize = unbufSize - payloadSize;
	}
}



void UdpChannel::recieveCallback(void* arg, struct udp_pcb* pcb,
		struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
	struct pbuf* pNext = p;
	UdpChannel* udpChannel = (UdpChannel*)arg;
	udpChannel->dmSource_.host = addr->addr;
	udpChannel->dmSource_.port = port;
	IVoidChannel::ConnectionParameter_t connParam = {
			.parameters = &(udpChannel->dmSource_),
			.parametersSize = sizeof(UdpHost_t)
	};
	if(udpChannel->callback_) {
		while(pNext){
			if(udpChannel->callback_){
				udpChannel->callback_->channelCallback((uint8_t*)pNext->payload,
						pNext->len, udpChannel, &connParam);
			}
			pNext = pNext->next;
		}
	}
	pbuf_free(p);
}



void UdpChannel::initialize(void* (* const mallocFunc)(size_t),
		const uint16_t txBufferSize, IChannelCallback* const callback)
{
	this->callback_ = callback;
}



void UdpChannel::deinitialize(void)
{
	this->callback_ = (IChannelCallback*)NULL;
}



void UdpChannel::getParameter(const uint8_t number, void* const value)
{

}



void UdpChannel::setParameter(const uint8_t number, void* const value)
{

}

void UdpChannel::setDestination(UdpHost_t* to) {
	this->dstIpaddr_.addr = to->host;
	this->dstPort_ = to->port;
}

uint32_t UdpChannel::getDestinationHost() {
	return this->dstIpaddr_.addr;
}

uint16_t UdpChannel::getDestinationPort() {
	return this->dstPort_;
}

}

#endif
