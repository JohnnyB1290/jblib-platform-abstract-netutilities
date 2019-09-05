/**
 * @file
 * @brief TCP Server void channel class Realization
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
#if USE_LWIP
#include <stdlib.h>
#include <string.h>
#include "jbkernel/jb_common.h"
#include "lwip/tcp.h"
#include "ethutilities/TcpServerChannel.hpp"
#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
#include <stdio.h>
#endif


namespace jblib::ethutilities
{

using namespace jbkernel;

typedef enum
{
  SERVER_STATE_NONE = 0,
  SERVER_STATE_ACCEPTED,
  SERVER_STATE_CLOSING
} TcpServerState_t;



typedef struct
{
  uint8_t state;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  TcpServerChannel* tcpServer;
}TcpServerParameters_t;



TcpServerChannel::TcpServerChannel(uint8_t* srcIp, uint16_t srcPort) : IVoidChannel()
{
	this->construct(srcIp, srcPort);
}



TcpServerChannel::TcpServerChannel(uint16_t srcPort) : IVoidChannel()
{
	this->construct(NULL, srcPort);
}



void TcpServerChannel::construct(uint8_t* srcIp, uint16_t srcPort)
{
	for(uint32_t i = 0; i < TCP_SERVER_NUM_BROADCAST_CONNECTIONS; i++)
		this->connectionsBuffer_[i] = NULL;
	if(srcIp)
		IP4_ADDR(&this->srcIpaddr_, srcIp[0], srcIp[1], srcIp[2], srcIp[3]);
	else
		this->srcIpaddr_ = ip_addr_any;
	this->srcPort_ = srcPort;
	this->mainPcb_ = tcp_new();
	if (this->mainPcb_) {
		err_t err = tcp_bind(this->mainPcb_, &this->srcIpaddr_, this->srcPort_);
		if (err == ERR_OK) {
			this->mainPcb_ = tcp_listen(this->mainPcb_);
			tcp_arg(this->mainPcb_, (void*)this);
			tcp_accept(this->mainPcb_, accept);
		}
		else {
			#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
			printf("TCP Server on port %u error: TCP Bind %i error! \r\n",
					this->srcPort_, err);
			#endif
		}
	}
	else {
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u error: Main PCB = NULL! \r\n",
				this->srcPort_);
		#endif
	}
}



err_t TcpServerChannel::accept(void* arg, struct tcp_pcb* newpcb, err_t err)
{
  /* Unless this pcb should have NORMAL priority, set its priority now.
     When running out of pcbs, low priority pcbs can be aborted to create
     new pcbs of higher priority. */
	tcp_setprio(newpcb, TCP_PRIO_NORMAL);

	err_t ret = ERR_OK;
	TcpServerParameters_t* ss =
			(TcpServerParameters_t*)malloc_s(sizeof(TcpServerParameters_t));
	if (ss != NULL) {
		ss->state = SERVER_STATE_ACCEPTED;
		ss->pcb = newpcb;
		ss->p = NULL;
		ss->tcpServer = (TcpServerChannel*)arg;
		tcp_arg(newpcb, ss);
		tcp_err(newpcb, TcpServerChannel::error);
		tcp_recv(newpcb, TcpServerChannel::receive);
		tcp_poll(newpcb, TcpServerChannel::poll, 0);
		ss->tcpServer->addConnectionToBuffer((void*)ss);
		ret = ERR_OK;
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server: New connection accepted on Port %u! \r\n",
				ss->tcpServer->srcPort_);
		#endif
	}
	else {
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u error: accept malloc ss! \r\n",
				ss->tcpServer->srcPort_);
		#endif
		ret = ERR_MEM;
	}
	return ret;
}



err_t TcpServerChannel::receive(void* arg, struct tcp_pcb* tpcb,
		struct pbuf* p, err_t err)
{
	LWIP_ASSERT("arg != NULL",arg != NULL);
	err_t ret = ERR_OK;
	TcpServerParameters_t* ss = (TcpServerParameters_t*)arg;
	if (p == NULL) {
		/* remote host closed connection */
		ss->state = SERVER_STATE_CLOSING;
		if(ss->p == NULL) {
			/* we're done sending, close it */
			close(tpcb, (void*)ss);
			#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
			printf("TCP Server: Remote host closed connection on port %u! "
					"Sending done \r\n"
					, ss->tcpServer->srcPort_);
			#endif
		}
		else {
			/* we're not done yet */
			tcp_sent(tpcb, TcpServerChannel::sent);
			ss->tcpServer->send(ss);
			#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
			printf("TCP Server: Remote host closed connection on port %u! "
					"Sending NOT done yet \r\n",
					ss->tcpServer->srcPort_);
			#endif
		}
		ret = ERR_OK;
	}
	else if(err != ERR_OK) {
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u receive error %i !\r\n",
				ss->tcpServer->srcPort_, err);
		#endif
		/* cleanup, for unkown reason */
		pbuf_free(p);
		ret = err;
	}
	else if(ss->state == SERVER_STATE_ACCEPTED) {
		struct pbuf* pNext = p;
		while(pNext){
			if(ss->tcpServer->callback_) {
				IVoidChannel::ConnectionParameter_t connParam = {
						.parameters = arg,
						.parametersSize = sizeof(TcpServerParameters_t)
				};
				ss->tcpServer->callback_->channelCallback((uint8_t*)pNext->payload,
						pNext->len, (void*)ss->tcpServer, &connParam);
			}
			pNext = pNext->next;
		}
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		ret = ERR_OK;
	}
	else if(ss->state == SERVER_STATE_CLOSING) {
		/* odd case, remote side closing twice, trash data */
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		ret = ERR_OK;
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u error: remote side closing twice, "
				"trash data! \r\n", ss->tcpServer->srcPort_);
		#endif
	}
	else {
		/* unkown es->state, trash data  */
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		ret = ERR_OK;
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u error: unkown ss->state, "
				"trash data! \r\n", ss->tcpServer->srcPort_);
		#endif
	}
	return ret;
}



err_t TcpServerChannel::poll(void* arg, struct tcp_pcb* tpcb)
{
	err_t ret = ERR_OK;
	TcpServerParameters_t* ss = (TcpServerParameters_t*)arg;
	if (ss) {
		if (ss->p){
			tcp_sent(tpcb, sent);
			ss->tcpServer->send(ss);
		}
		else {
			/* no remaining pbuf (chain)  */
			if(ss->state == SERVER_STATE_CLOSING) {
				close(tpcb, (void*)ss);
				#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
				printf("TCP Server on port %i: POLL Closing\r\n",
						ss->tcpServer->srcPort_);
				#endif
			}
		}
		ret = ERR_OK;
	}
	else {
		/* nothing to be done */
		tcp_abort(tpcb);
		ret = ERR_ABRT;
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u error: POLL ss == NULL!\r\n",
				ss->tcpServer->srcPort_);
		#endif
	}
	return ret;
}



err_t TcpServerChannel::sent(void* arg, struct tcp_pcb* tpcb, u16_t len)
{
	TcpServerParameters_t* ss = (TcpServerParameters_t*)arg;
	if(ss->p) {
		/* still got pbufs to send */
		tcp_sent(tpcb, sent);
		ss->tcpServer->send(ss);
	}
	else {
		/* no more pbufs to send */
		if(ss->state == SERVER_STATE_CLOSING)
			close(tpcb, ss);
	}
	return ERR_OK;
}



void TcpServerChannel::send(void* arg)
{
	err_t writeError = ERR_OK;
	TcpServerParameters_t* ss = (TcpServerParameters_t*)arg;
	while ((writeError == ERR_OK) && (ss->p != NULL) &&
			(ss->p->len <= tcp_sndbuf(ss->pcb))) {

		struct pbuf* p = ss->p;
		/* enqueue data for transmission */
		writeError = tcp_write(ss->pcb, p->payload, p->len, 1);
		if (writeError == ERR_OK) {
			/* continue with next pbuf in chain (if any) */
			ss->p = p->next;
			if(ss->p)
				pbuf_ref(ss->p); /* new reference! */
			/* chop first pbuf from chain */
			u8_t freed = 0;
			while(freed == 0)
				freed = pbuf_free(p); /* try hard to free pbuf */
		}
		else if(writeError == ERR_MEM) {
			/* we are low on memory, try later / harder, defer to poll */
			ss->p = p;
			#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
			printf("TCP Server on port %u error: send NO MEM! Try later\r\n",
					ss->tcpServer->srcPort_);
			#endif
		}
		else {
			/* other problem ?? */
			#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
			printf("TCP Server on port %u error: send Undefined! \r\n",
					ss->tcpServer->srcPort_);
			#endif
		}
	}
}



void TcpServerChannel::tx(uint8_t* const buffer, const uint16_t size, void* parameter)
{
	if(this->connectionsCounter_ == 0)
		return;
	if(parameter== NULL) {
		IVoidChannel::ConnectionParameter_t connParam = {
				.parameters = NULL,
				.parametersSize = sizeof(TcpServerParameters_t)
		};
		for(uint32_t i = 0; i < this->connectionsCounter_; i++) {
			if(this->connectionsBuffer_[i]){
				connParam.parameters = this->connectionsBuffer_[i];
				this->tx(buffer, size, &connParam);
			}
		}
		return;
	}
	TcpServerParameters_t* ss =
			(TcpServerParameters_t*)(((IVoidChannel::ConnectionParameter_t*)parameter)->parameters);
	if((!this->isConnectionInBuffer(ss)) || (ss->p != NULL))
		return;
	uint8_t* data = buffer;
	uint32_t unbufferedSize = size;
	while(unbufferedSize > 0) {
		uint32_t  payloadSize = MIN(unbufferedSize, TCP_MSS);
		struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, payloadSize, PBUF_RAM);
		if (p == NULL)
			break;
		memcpy(p->payload, data, payloadSize);
		data = data + payloadSize*sizeof(uint8_t);
		if(ss->p == NULL)
			ss->p = p;
		else
			pbuf_chain(ss->p,p);
		unbufferedSize = unbufferedSize - payloadSize;
	}
	this->send(ss);
}



void TcpServerChannel::error(void* arg, err_t err)
{
	#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
	TcpServerParameters_t* ss = (TcpServerParameters_t*)arg;
	printf("TCP Server on port %u error: error %i! \r\n",
			ss->tcpServer->srcPort_, err);
	#endif
	TcpServerParameters_t* ss = (TcpServerParameters_t*)arg;
	if (ss){
		ss->tcpServer->deleteConnectionFromBuffer(arg);
		free_s(arg);
	}
}



void TcpServerChannel::close(struct tcp_pcb* tpcb, void* arg)
{
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);
	TcpServerParameters_t* ss = (TcpServerParameters_t*)arg;
	if (ss){
		ss->tcpServer->deleteConnectionFromBuffer(arg);
		free_s(arg);
	}
	else {
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u error: close ss = NULL connection! \r\n",
				ss->tcpServer->srcPort_);
		#endif
	}
	tcp_close(tpcb);
}



TcpServerChannel::~TcpServerChannel(void)
{
	tcp_abort(this->mainPcb_);
}



void TcpServerChannel::initialize(void* (* const mallocFunc)(size_t),
		const uint16_t txBufferSize, IChannelCallback* const callback)
{
	this->callback_ = callback;
}



void TcpServerChannel::deinitialize(void)
{
	this->callback_ = (IChannelCallback*)NULL;
}



void TcpServerChannel::getParameter(const uint8_t number, void* const value)
{

}



void TcpServerChannel::setParameter(const uint8_t number, void* const value)
{

}



void TcpServerChannel::addConnectionToBuffer(void* ss)
{
	if(this->connectionsCounter_ == TCP_SERVER_NUM_BROADCAST_CONNECTIONS) {
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u: Max number of broadcast connections "
				"achieved = %lu!\r\n",this->srcPort_,
				this->connectionsCounter_);
		#endif
	}
	else {
		for(uint32_t i = 0; i < TCP_SERVER_NUM_BROADCAST_CONNECTIONS; i++) {
			if(this->connectionsBuffer_[i] == ss)
				break;
			if(this->connectionsBuffer_[i] == NULL) {
				this->connectionsBuffer_[i] = ss;
				break;
			}
		}
		this->connectionsCounter_++;
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u: Number of connections = %lu!\r\n",
				this->srcPort_,
				this->connectionsCounter_);
		#endif
	}
}



void TcpServerChannel::deleteConnectionFromBuffer(void* ss)
{
	if(this->connectionsCounter_){
		uint32_t index = 0;
		for(uint32_t i = 0; i < TCP_SERVER_NUM_BROADCAST_CONNECTIONS; i++) {
			if(this->connectionsBuffer_[i] == ss)
				break;
			else
				index++;
		}
		if(index == (TCP_SERVER_NUM_BROADCAST_CONNECTIONS-1)) {
			if(this->connectionsBuffer_[index] == ss)
				this->connectionsBuffer_[index] = NULL;
		}
		else {
			for(uint32_t i = index; i < (TCP_SERVER_NUM_BROADCAST_CONNECTIONS-1); i++) {
				this->connectionsBuffer_[i] = this->connectionsBuffer_[i+1];
				if(this->connectionsBuffer_[i+1] == NULL)
					break;
			}
		}
		this->connectionsCounter_--;
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u: Number of connections = %lu!\r\n",
				this->srcPort_,
				this->connectionsCounter_);
		#endif
	}
	else {
		#if (USE_CONSOLE && TCP_SERVER_USE_CONSOLE)
		printf("TCP Server on port %u error: "
				"Delete from connection buffer, but counter = 0! \r\n",
				this->srcPort_);
		#endif
		return;
	}
}



bool TcpServerChannel::isConnectionInBuffer(void* ss)
{
	for(uint32_t i = 0; i < this->connectionsCounter_; i++) {
		if(this->connectionsBuffer_[i] == ss)
			return true;
	}
	return false;
}

}

#endif
