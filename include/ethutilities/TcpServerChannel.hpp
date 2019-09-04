/**
 * @file
 * @brief TCP Server void channel class Description
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

#ifndef TCP_SERVER_CHANNEL_HPP_
#define TCP_SERVER_CHANNEL_HPP_

#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "jbkernel/IVoidChannel.hpp"

namespace jblib
{
namespace ethutilities
{

using namespace jbkernel;

class TcpServerChannel : public IVoidChannel
{
public:
	TcpServerChannel(uint8_t* srcIp, uint16_t srcPort);
	TcpServerChannel(uint16_t srcPort);
	virtual ~TcpServerChannel(void);
	virtual void initialize(void* (* const mallocFunc)(size_t),
			const uint16_t txBufferSize, IChannelCallback* const callback);
	virtual void deinitialize(void);
	virtual void tx(uint8_t* const buffer, const uint16_t size, void* parameter);
	virtual void getParameter(const uint8_t number, void* const value);
	virtual void setParameter(const uint8_t number, void* const value);

private:
	static err_t poll(void* arg, struct tcp_pcb* tpcb);
	static err_t accept(void* arg, struct tcp_pcb* newpcb, err_t err);
	static void error(void* arg, err_t err);
	static void close(struct tcp_pcb* tpcb, void* arg);
	static err_t receive(void* arg, struct tcp_pcb* tpcb, struct pbuf* p,
			err_t err);
	static err_t sent(void* arg, struct tcp_pcb* tpcb, u16_t len);
	void construct(uint8_t* srcIp, uint16_t srcPort);
	void send(void* arg);
	void addConnectionToBuffer(void* ss);
	void deleteConnectionFromBuffer(void* ss);
	bool isConnectionInBuffer(void* ss);

	struct tcp_pcb* mainPcb_ = NULL;
	ip_addr_t srcIpaddr_ = {.addr = 0};
	uint16_t srcPort_ = 0;
	IChannelCallback* callback_ = NULL;
	uint32_t connectionsCounter_ = 0;
	void* connectionsBuffer_[TCP_SERVER_NUM_BROADCAST_CONNECTIONS];
};

}
}

#endif /* TCP_SERVER_CHANNEL_HPP_ */
