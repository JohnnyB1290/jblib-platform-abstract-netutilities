/**
 * @file
 * @brief WebSocket void channel class Description
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

#ifndef ETHUTILITIES_WSCHANNEL_HPP_
#define ETHUTILITIES_WSCHANNEL_HPP_

#include "jbkernel/jb_common.h"
#include "jbkernel/IVoidChannel.hpp"
#include "jbutilities/LinkedList.hpp"
#include "httpd.h"



namespace jblib
{
namespace ethutilities
{

using namespace jbkernel;
using namespace jbutilities;


typedef struct
{
	struct tcp_pcb* pcb = NULL;
	uint8_t mode = 0;
}WsConnectInfo_t;



class WsChannel : public IVoidChannel
{
public:
	static WsChannel* createWsChannel(char* uri);
	static WsChannel* getWsChannel(char* uri);
	virtual void initialize(void* (* const mallocFunc)(size_t),
			const uint16_t txBufferSize, IChannelCallback* const callback);
	virtual void deinitialize(void);
	virtual void tx(uint8_t* const buffer, const uint16_t size, void* parameter);
	virtual void getParameter(const uint8_t number, void* const value);
	virtual void setParameter(const uint8_t number, void* const value);

private:
	static void websocketCallback(struct tcp_pcb* pcb, uint8_t* data,
			u16_t datalen, uint8_t mode);
	static void websocketOpenCallback(struct tcp_pcb* pcb, const char* uri);
	WsChannel(char* uri);
	bool checkUri(const char* uri);
	bool checkPcb(struct tcp_pcb* pcb);

	static LinkedList<WsChannel>* wsChannelsList_;
	char uri_[WS_CHANNEL_URI_MAX_SIZE];
	struct tcp_pcb* pcbs_[WS_CHANNEL_MAX_NUM_CONNECTIONS];
	WsConnectInfo_t wsConnectInfo_;
	IChannelCallback* callback_ = NULL;
};

}
}



#endif /* ETHUTILITIES_WSCHANNEL_HPP_ */
