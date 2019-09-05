/**
 * @file
 * @brief Ethernet Router class Realization
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
#include "arch/systick_arch.h"
#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "arch/ethernetif.h"
#include "jbdrivers/JbController.hpp"
#include "ethutilities/EthernetRouter.hpp"
#include "jbkernel/TimeEngine.hpp"
#if (USE_CONSOLE && ETHERNET_ROUTER_USE_CONSOLE)
#include <stdio.h>
#endif

namespace jblib
{
namespace ethutilities
{

using namespace jbkernel;

typedef enum
{
	CALLBACK_TYPE_LWIP_MS = 0,
	CALLBACK_TYPE_LINK_CHECK = 1,
	CALLBACK_TYPE_ROUTE_FRAMES = 2
}RouterCallbackType_t;



EthernetRouter* EthernetRouter::ethernetRouter_ = (EthernetRouter*)NULL;



EthernetRouter* EthernetRouter::getEthernetRouter(void)
{
	if(ethernetRouter_ == (EthernetRouter*)NULL)
		ethernetRouter_ = new EthernetRouter();
	return ethernetRouter_;
}



EthernetRouter::EthernetRouter(void) : IVoidCallback()
{
	memset(&this->inputFrame_, 0, sizeof(EthernetFrame));
	#if (USE_CONSOLE && ETHERNET_ROUTER_USE_CONSOLE)
	printf("Ethernet router: Starting LWIP Library\r\n");
	#endif
	lwip_init();
}



std::forward_list<EthernetRouterIface_t*>* EthernetRouter::getInterfacesList(void)
{
	return &this->interfacesList_;
}



void EthernetRouter::setParsePeriodUs(uint32_t parsePeriodUs)
{
	this->parsePeriodUs_ = parsePeriodUs;
}



EthernetRouterIface_t* EthernetRouter::addInterface(IVoidEthernet* interface, uint8_t* ip,
		uint8_t* gateway, uint8_t* netmask)
{
	EthernetRouterIface_t* newIface = new EthernetRouterIface_t();
	if(newIface) {
		newIface->netifPtr = new struct netif();
		newIface->interface = interface;
		newIface->interface->initialize();
		newIface->interface->start();
		newIface->arpController = new ArpController(newIface->interface);
		newIface->arpController->addIpForArpReply(ip, netmask);

		memcpy(newIface->ip, ip, 4);
		memcpy(newIface->gateway, gateway, 4);
		memcpy(newIface->netmask, netmask, 4);
		newIface->interface->getParameter(PARAMETER_MAC ,&newIface->mac);

		ip_addr_t ipaddr, mask, gw;
		IP4_ADDR(&ipaddr, ip[0], ip[1], ip[2], ip[3]);
		IP4_ADDR(&gw, gateway[0], gateway[1], gateway[2], gateway[3]);
		IP4_ADDR(&mask, netmask[0], netmask[1], netmask[2], netmask[3]);

		#if (USE_CONSOLE && ETHERNET_ROUTER_USE_CONSOLE)
		char* adapterName = NULL;
		newIface->interface->getParameter(PARAMETER_NAME, (void*)&adapterName);
		printf("Ethernet router: %s \r\n", adapterName);
		printf("IP = %d.%d.%d.%d\r\n",
				ip4_addr1(&ipaddr),ip4_addr2(&ipaddr),
				ip4_addr3(&ipaddr), ip4_addr4(&ipaddr));
		printf("Gateway = %d.%d.%d.%d\r\n",
				ip4_addr1(&gw), ip4_addr2(&gw),
				ip4_addr3(&gw), ip4_addr4(&gw));
		printf("Netmask = %d.%d.%d.%d\r\n\r\n",
				ip4_addr1(&mask), ip4_addr2(&mask),
				ip4_addr3(&mask), ip4_addr4(&mask));
		#endif

		netif_add(newIface->netifPtr, &ipaddr, &mask, &gw,
					(void*)newIface->interface, lwipEthernetifInit, netif_input);
		netif_set_up(newIface->netifPtr);

		this->interfacesList_.push_front(newIface);
	}
	return newIface;
}



void EthernetRouter::start(uint8_t nrtTimerNumber, bool routeFramesInTimer, bool checkLink)
{
	static bool isStarted = false;
	if(!isStarted){
		this->nrtTimerNumber_ = nrtTimerNumber;
		this->routeFramesInTimer_ = routeFramesInTimer;
		this->checkLink_ = checkLink;
		if(this->routeFramesInTimer_){
			TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
					1000, this, (void*)CALLBACK_TYPE_ROUTE_FRAMES);
		}
		else
			jbdrivers::JbController::addMainProcedure(this);
		TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
				1000, this, (void*)CALLBACK_TYPE_LWIP_MS);
		TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
				1000000, this, (void*)CALLBACK_TYPE_LINK_CHECK);
		isStarted = true;
	}
}



void EthernetRouter::start(uint8_t nrtTimerNumber, bool routeFramesInTimer)
{
	this->start(nrtTimerNumber, routeFramesInTimer, true);
}



EthernetRouterIface_t* EthernetRouter::getEthRouterInterface(IVoidEthernet* interface)
{
	for(std::forward_list<EthernetRouterIface_t*>::iterator it = this->interfacesList_.begin();
			it != this->interfacesList_.end(); ++it){
		EthernetRouterIface_t* routerIface = *it;
		if(routerIface->interface == interface)
			return routerIface;
	}
	return NULL;
}



void EthernetRouter::voidCallback(void* const source, void* parameter)
{
	if(source == TimeEngine::getTimeEngine()){
		TimeEngineCallbackParameters_t* timeEngineParameters =
				(TimeEngineCallbackParameters_t*)parameter;
		if(this->nrtTimerNumber_ == timeEngineParameters->timerNumber){
			if(timeEngineParameters->data == (void*)CALLBACK_TYPE_ROUTE_FRAMES){
				this->routeFrames();
			}
			else if(timeEngineParameters->data == (void*)CALLBACK_TYPE_LWIP_MS) {
				lwipTimerHandler();
				TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
						1000, this, (void*)CALLBACK_TYPE_LWIP_MS);
			}
			else if(timeEngineParameters->data == (void*)CALLBACK_TYPE_LINK_CHECK) {
				for(std::forward_list<EthernetRouterIface_t*>::iterator it = this->interfacesList_.begin();
						it != this->interfacesList_.end(); ++it){
					EthernetRouterIface_t* routerIface = *it;
					if(this->checkLink_ || (routerIface->link == 0)) {
						routerIface->interface->getParameter(PARAMETER_LINK, &(routerIface->link));
						if(routerIface->link)
							netif_set_link_up(routerIface->netifPtr);
						else
							netif_set_link_down(routerIface->netifPtr);
					}
					routerIface->arpController->voidCallback(this, NULL);
				}
				TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
						1000000, this, (void*)CALLBACK_TYPE_LINK_CHECK);
			}
		}
	}
	else
		this->routeFrames();
}



void EthernetRouter::routeFrames(void)
{
	if(this->interfacesList_.empty()){
		if(this->routeFramesInTimer_){
			TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
					100000, this, (void*)CALLBACK_TYPE_ROUTE_FRAMES);
		}
		return;
	}
	for(std::forward_list<EthernetRouterIface_t*>::iterator it = this->interfacesList_.begin();
			it != this->interfacesList_.end(); ++it){
		EthernetRouterIface_t* routerIface = *it;
		this->inputFrameSize_ = routerIface->interface->pullOutRxFrame(&this->inputFrame_);
		if(this->inputFrameSize_) {
			uint8_t* frameU8 = (uint8_t*)&this->inputFrame_;
			if(EthernetUtilities::getFrameType(frameU8) == ETX_ETHER_TYPE_ARP){
				routerIface->arpController->parseFrame(&this->inputFrame_, this->inputFrameSize_,
						routerIface->interface, NULL);
				if(memcmp(&frameU8[ETX_ARP_TARGET_IP_OFFSET], routerIface->ip, ETX_PROTO_SIZE) == 0){
					this->pushFrameToLwip(routerIface->netifPtr);
				}
			}
			else{
				if(memcmp(&frameU8[ETX_ETH_D_MAC_OFFSET], routerIface->mac, ETX_HW_SIZE) == 0){
					if(memcmp(&frameU8[ETX_IP_DIP_OFFSET], routerIface->ip, ETX_PROTO_SIZE) == 0){
						this->pushFrameToLwip(routerIface->netifPtr);
					}
					for(std::forward_list<IEthernetListener*>::iterator listenersIt =
							routerIface->listenersList.begin();
							listenersIt != routerIface->listenersList.end(); ++listenersIt){
						(*listenersIt)->parseFrame(&this->inputFrame_, this->inputFrameSize_,
								routerIface->interface, routerIface);
					}
				}
			}
		}
	}
	disableInterrupts();
	sys_check_timeouts();
	enableInterrupts();
	if(this->routeFramesInTimer_){
		TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
				this->parsePeriodUs_, this, (void*)CALLBACK_TYPE_ROUTE_FRAMES);
	}
}



void EthernetRouter::pushFrameToLwip(struct netif* netifPtr)
{
	disableInterrupts();
	lwipEthernetifInput(netifPtr, &this->inputFrame_, this->inputFrameSize_);
	enableInterrupts();
}



void EthernetRouter::addListener(IEthernetListener* listener,
		EthernetRouterIface_t* routerInterface)
{
	if(routerInterface)
		routerInterface->listenersList.push_front(listener);
}



void EthernetRouter::addListener(IEthernetListener* listener, IVoidEthernet* interface)
{
	for(std::forward_list<EthernetRouterIface_t*>::iterator it = this->interfacesList_.begin();
			it != this->interfacesList_.end(); ++it){
		if((*it)->interface == interface)
			this->addListener(listener, (*it));
	}
}



void EthernetRouter::addListener(IEthernetListener* listener)
{
	for(std::forward_list<EthernetRouterIface_t*>::iterator it = this->interfacesList_.begin();
			it != this->interfacesList_.end(); ++it){
		this->addListener(listener, (*it));
	}
}



void EthernetRouter::deleteListener(IEthernetListener* listener,
		EthernetRouterIface_t* routerInterface)
{
	routerInterface->listenersList.remove_if([listener](IEthernetListener* current){
		if(current == listener)
			return true;
		else
			return false;
	});
}



void EthernetRouter::deleteListener(IEthernetListener* listener,
		IVoidEthernet* interface)
{
	for(std::forward_list<EthernetRouterIface_t*>::iterator it = this->interfacesList_.begin();
			it != this->interfacesList_.end(); ++it){
		if((*it)->interface == interface)
			this->deleteListener(listener, (*it));
	}
}



void EthernetRouter::deleteListener(IEthernetListener* listener)
{
	for(std::forward_list<EthernetRouterIface_t*>::iterator it = this->interfacesList_.begin();
			it != this->interfacesList_.end(); ++it){
		this->deleteListener(listener, (*it));
	}
}

}
}

#endif
