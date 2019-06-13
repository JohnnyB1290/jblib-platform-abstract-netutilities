/**
 * @file
 * @brief Ethernet Router class Description
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


#ifndef ETHERNET_ROUTER_HPP_
#define ETHERNET_ROUTER_HPP_

#include "jbkernel/jb_common.h"
#include "lwip/netif.h"
#include "jbkernel/callback_interfaces.hpp"
#include "jbkernel/IVoidEthernet.hpp"
#include "ethutilities/ArpController.hpp"


namespace jblib
{
namespace ethutilities
{

using namespace jbkernel;

#pragma pack(push, 1)
typedef struct EthernetRouterIface_t
{
	EthernetRouterIface_t* next = NULL;
	IVoidEthernet* interface = NULL;
	struct netif* netifPtr = NULL;
	ArpController* arpController = NULL;
	IEthernetListener* listeners[ETHERNET_ROUTER_MAX_NUM_LISTENERS];
	uint8_t ip[4];
	uint8_t gateway[4];
	uint8_t netmask[4];
	uint8_t* mac = NULL;
	uint32_t link = 0;
}EthernetRouterIface_t;
#pragma pack(pop)



class EthernetRouter : public IVoidCallback
{
public:
	static EthernetRouter* getEthernetRouter(void);
	EthernetRouterIface_t* getEthRouterInterface(IVoidEthernet* interface);
	EthernetRouterIface_t* addInterface(IVoidEthernet* interface, uint8_t* ip,
				uint8_t* gateway, uint8_t* netmask);
	void start(uint8_t nrtTimerNumber, bool routeFramesInTimer);
	void start(uint8_t nrtTimerNumber, bool routeFramesInTimer, bool checkLink);
	void addListener(IEthernetListener* listener);
	void addListener(IEthernetListener* listener, IVoidEthernet* interface);
	void addListener(IEthernetListener* listener, EthernetRouterIface_t* routerInterface);
	void deleteListener(IEthernetListener* listener);
	void deleteListener(IEthernetListener* listener,
			IVoidEthernet* interface);
	void deleteListener(IEthernetListener* listener, EthernetRouterIface_t* routerInterface);
	virtual void voidCallback(void* const source, void* parameter);
	void setParsePeriodUs(uint32_t parsePeriodUs);


private:
	EthernetRouter(void);
	void routeFrames(void);
	void pushFrameToLwip(struct netif* netifPtr);

	static EthernetRouter* ethernetRouter_;
	EthernetRouterIface_t* firstInterface_ = NULL;
	EthernetFrame inputFrame_;
	uint16_t inputFrameSize_ = 0;
	uint8_t nrtTimerNumber_ = 0;
	bool routeFramesInTimer_ = false;
	bool checkLink_ = true;
	uint32_t parsePeriodUs_ = ETHERNET_ROUTER_DEFAULT_PARSE_PERIOD_US;
};

}
}

#endif /* ETHERNET_ROUTER_HPP_ */
