/**
 * @file
 * @brief WebSocket void channel class Realization
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
#include <string.h>
#include "ethutilities/WsChannel.hpp"



namespace jblib::ethutilities
{

using namespace jbkernel;
using namespace jbutilities;


LinkedList<WsChannel>* WsChannel::wsChannelsList_ = NULL;



WsChannel* WsChannel::createWsChannel(char* uri)
{
	if(wsChannelsList_ == NULL){
		wsChannelsList_ = new LinkedList<WsChannel>();
		websocket_register_callbacks(websocketOpenCallback, websocketCallback);
	}
	LinkedList<WsChannel>::LinkIterator* iterator =
			wsChannelsList_->getIterator();
	iterator->reset();
	uint32_t listSize = wsChannelsList_->getSize();
	for(uint32_t i = 0; i < listSize; i++){
		WsChannel* channel = iterator->getCurrent();
		iterator->nextLink();
		if(channel->checkUri(uri))
			return channel;
	}
	WsChannel* newChannel = new WsChannel(uri);
	wsChannelsList_->insertLast(newChannel);
	return newChannel;
}



WsChannel* WsChannel::getWsChannel(char* uri)
{
	if(wsChannelsList_ == NULL)
		return NULL;
	if(uri){
		LinkedList<WsChannel>::LinkIterator* iterator =
					wsChannelsList_->getIterator();
		iterator->reset();
		uint32_t listSize = wsChannelsList_->getSize();
		for(uint32_t i = 0; i < listSize; i ++){
			WsChannel* channel = iterator->getCurrent();
			iterator->nextLink();
			if(channel->checkUri(uri))
				return channel;
		}
	}
	else{
		if(wsChannelsList_)
			return wsChannelsList_->getFirst();
	}
	return NULL;
}



WsChannel::WsChannel(char* uri)
{
	for(uint32_t i = 0; i < WS_CHANNEL_MAX_NUM_CONNECTIONS; i++)
		this->pcbs_[i] = NULL;
	memset(this->uri_, 0, WS_CHANNEL_URI_MAX_SIZE);
	strncpy(this->uri_, uri, WS_CHANNEL_URI_MAX_SIZE);
}



bool WsChannel::checkUri(const char* uri)
{
	if(!strncmp(this->uri_, uri, WS_CHANNEL_URI_MAX_SIZE))
		return true;
	else
		return false;
}



bool WsChannel::checkPcb(struct tcp_pcb* pcb)
{
	for(uint32_t i = 0; i < WS_CHANNEL_MAX_NUM_CONNECTIONS; i++){
		if(this->pcbs_[i] == pcb)
			return true;
	}
	return false;
}



void WsChannel::initialize(void* (* const mallocFunc)(size_t),
		const uint16_t txBufferSize, IChannelCallback* const callback)
{
	this->callback_ = callback;
}



void WsChannel::deinitialize(void)
{

}



void WsChannel::tx(uint8_t* const buffer, const uint16_t size, void* parameter)
{
	if(parameter == NULL) {
		for(uint32_t i = 0; i < WS_CHANNEL_MAX_NUM_CONNECTIONS; i++) {
			if(this->pcbs_[i]){
				if(this->pcbs_[i]->state != ESTABLISHED)
					this->pcbs_[i] = NULL;
				else if(buffer != NULL) {
					__disable_irq();
					websocket_write(this->pcbs_[i], buffer, size, WS_BIN_MODE);
					__enable_irq();
				}

			}
		}
	}
	else if(buffer != NULL){
		__disable_irq();
		WsConnectInfo_t* wsConnectInfo = (WsConnectInfo_t*)parameter;
		websocket_write(wsConnectInfo->pcb, buffer, size, wsConnectInfo->mode);
		__enable_irq();
	}
}



void WsChannel::getParameter(const uint8_t number, void* const value)
{

}



void WsChannel::setParameter(const uint8_t number, void* const value)
{

}


void WsChannel::websocketOpenCallback(struct tcp_pcb* pcb, const char* uri)
{
	if(wsChannelsList_ == NULL)
		return;
	LinkedList<WsChannel>::LinkIterator* iterator =
				wsChannelsList_->getIterator();
	iterator->reset();
	uint32_t listSize = wsChannelsList_->getSize();
	for(uint32_t i = 0; i < listSize; i ++){
		WsChannel* channel = iterator->getCurrent();
		iterator->nextLink();
		if(channel->checkUri(uri)){
			for(uint32_t j = 0; j < WS_CHANNEL_MAX_NUM_CONNECTIONS; j++){
				if (channel->pcbs_[j] == NULL){
					channel->pcbs_[j] = pcb;
					break;
				}
				else if(channel->pcbs_[j]->state != ESTABLISHED){
					channel->pcbs_[j] = pcb;
					break;
				}
			}
		}
	}
}



void WsChannel::websocketCallback(struct tcp_pcb* pcb, uint8_t* data,
		u16_t datalen, uint8_t mode)
{
	if(wsChannelsList_ == NULL)
		return;
	LinkedList<WsChannel>::LinkIterator* iterator =
				wsChannelsList_->getIterator();
	iterator->reset();
	uint32_t listSize = wsChannelsList_->getSize();
	for(uint32_t i = 0; i < listSize; i ++){
		WsChannel* channel = iterator->getCurrent();
		iterator->nextLink();
		if(channel->checkPcb(pcb)){
			channel->wsConnectInfo_.mode = mode;
			channel->wsConnectInfo_.pcb = pcb;
			if(channel->callback_){
				channel->callback_->channelCallback(data, datalen,
					channel, &channel->wsConnectInfo_);
			}
		}
	}
}

}

#endif
