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

#include "jb_common.h"
#include "lwip/netif.h"
#include "callback_interfaces.hpp"
#include "IVoidEthernet.hpp"
#include "ArpController.hpp"

namespace jblib::ethutilities
{

using namespace jbkernel;

class EthernetRouter : public IVoidCallback
{
public:
	static EthernetRouter* getEthernetRouter(void);
	ArpController* getArpController(IVoidEthernet* interface);
	void start(uint8_t nrtTimerNumber, bool routeFramesInTimer);
	struct netif* getNetif(IVoidEthernet* interface);
	void addInterface(IVoidEthernet* interface, uint8_t* ip,
			uint8_t* gateway, uint8_t* netmask);
	void addListener(IEthernetListener* listener, IVoidEthernet* interface);
	void addListener(IEthernetListener* listener);
	void deleteListener(IEthernetListener* listener,
			IVoidEthernet* interface);
	void deleteListener(IEthernetListener* listener);
	virtual void voidCallback(void* const source, void* parameter);
	void setParsePeriodUs(uint32_t parsePeriodUs);

private:
	EthernetRouter(void);
	void routeFrames(void);

	static EthernetRouter* ethernetRouter_;
	IVoidEthernet* interfaces_[ETHERNET_ROUTER_MAX_NUM_INTERFACES];
	struct netif netifs_[ETHERNET_ROUTER_MAX_NUM_INTERFACES];
	ArpController* arpControllers_[ETHERNET_ROUTER_MAX_NUM_INTERFACES];
	uint8_t interfacesCounter_ = 0;
	EthernetFrame inputFrame_;
	uint16_t inputFrameSize_ = 0;
	IEthernetListener* listeners_[ETHERNET_ROUTER_MAX_NUM_INTERFACES]
								 [ETHERNET_ROUTER_MAX_NUM_LISTENERS];
	uint8_t nrtTimerNumber_ = 0;
	bool routeFramesInTimer_ = false;
	uint32_t parsePeriodUs_ = ETHERNET_ROUTER_DEFAULT_PARSE_PERIOD_US;
};

}

#endif /* ETHERNET_ROUTER_HPP_ */
