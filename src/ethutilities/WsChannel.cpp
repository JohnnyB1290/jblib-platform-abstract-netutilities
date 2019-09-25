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
#if USE_LWIP && USE_WEB_SERVER
#include <string.h>
#include "ethutilities/WsChannel.hpp"



namespace jblib::ethutilities
{

using namespace jbkernel;
using namespace jbutilities;


std::forward_list<WsChannel*> WsChannel::wsChannelsList_;



WsChannel* WsChannel::createWsChannel(char* uri)
{
	static bool isCallbacksRegistered = false;
	if(!isCallbacksRegistered){
		websocket_register_callbacks(websocketOpenCallback, websocketCallback);
		isCallbacksRegistered = true;
	}
	if(!uri)
		return NULL;
	WsChannel* ws = getWsChannel(uri);
	if(!ws){
		ws = new WsChannel(uri);
		wsChannelsList_.push_front(ws);
	}
	return ws;
}



WsChannel* WsChannel::getWsChannel(const char* uri)
{
	if(!uri)
		return NULL;
	for(std::forward_list<WsChannel*>::iterator it = wsChannelsList_.begin();
			it != wsChannelsList_.end(); ++it){
		WsChannel* wsChannel = *it;
		if(!wsChannel->uri_.compare(uri))
			return wsChannel;
	}
	return NULL;
}



WsChannel::WsChannel(char* uri)
{
	this->uri_ = uri;
}



void WsChannel::checkPcbs(void)
{
	this->pcbsList_.remove_if([](struct tcp_pcb* pcb){
		if(pcb->state != ESTABLISHED)
			return true;
		else
			return false;
	});
}



void WsChannel::initialize(void* (* const mallocFunc)(size_t),
		const uint16_t txBufferSize, IChannelCallback* const callback)
{
	this->callback_ = callback;
}



void WsChannel::deinitialize(void)
{
	this->callback_ = NULL;
}



void WsChannel::tx(uint8_t* const buffer, const uint16_t size, void* parameter)
{
	this->checkPcbs();
	if(!buffer)
		return;
	if(!parameter) {
		for(std::forward_list<struct tcp_pcb*>::iterator it = this->pcbsList_.begin();
				it != this->pcbsList_.end(); ++it){
			disableInterrupts();
			websocket_write(*it, buffer, size, WS_BIN_MODE);
			enableInterrupts();
		}
	}
	else{
		disableInterrupts();
		WsConnectInfo_t* wsConnectInfo =
				(WsConnectInfo_t*)(((IVoidChannel::ConnectionParameter_t*)parameter)->parameters);
		websocket_write(wsConnectInfo->pcb, buffer, size, wsConnectInfo->mode);
		enableInterrupts();
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
	WsChannel* channel = getWsChannel(uri);
	if(channel){
		channel->checkPcbs();
		channel->pcbsList_.push_front(pcb);
	}
}



void WsChannel::websocketCallback(struct tcp_pcb* pcb, uint8_t* data,
		u16_t datalen, uint8_t mode)
{
	WsConnectInfo_t connectInfo;
	IVoidChannel::ConnectionParameter_t connParameter = {
			.parameters = &connectInfo,
			.parametersSize = sizeof(WsConnectInfo_t)
	};
	for(std::forward_list<WsChannel*>::iterator it = wsChannelsList_.begin();
			it != wsChannelsList_.end(); ++it){
		WsChannel* wsChannel = *it;
		for(std::forward_list<struct tcp_pcb*>::iterator pcbIt = wsChannel->pcbsList_.begin();
			pcbIt != wsChannel->pcbsList_.end(); ++pcbIt){
			if(*pcbIt == pcb){
				connectInfo.mode = mode;
				connectInfo.pcb = pcb;
				if(wsChannel->callback_){
					wsChannel->callback_->channelCallback(data, datalen,
							wsChannel, &connParameter);
				}
			}
		}
	}
}

}

#endif
