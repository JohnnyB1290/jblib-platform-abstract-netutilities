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
					(void*)newIface->interface, lwip_ethernetif_init, ethernet_input);
		netif_set_up(newIface->netifPtr);

		for(uint32_t i = 0; i < ETHERNET_ROUTER_MAX_NUM_LISTENERS; i++)
			newIface->listeners[i] = NULL;

		if(this->firstInterface_ == NULL){
			this->firstInterface_ = newIface;
		}
		else{
			EthernetRouterIface_t* ptr = this->firstInterface_;
			while(ptr->next != NULL)
				ptr = ptr->next;
			ptr->next = newIface;
		}
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
	if(interface == NULL)
		return this->firstInterface_;
	EthernetRouterIface_t* ptr = this->firstInterface_;
	while(ptr != NULL) {
		if(ptr->interface == interface)
			return ptr;
		ptr = ptr->next;
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
				LWIPTimer_Handler();
				TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
						1000, this, (void*)CALLBACK_TYPE_LWIP_MS);
			}
			else if(timeEngineParameters->data == (void*)CALLBACK_TYPE_LINK_CHECK) {

				EthernetRouterIface_t* ptr = this->firstInterface_;
				while(ptr != NULL){
					if(this->checkLink_ || (ptr->link == 0)) {
						ptr->interface->getParameter(PARAMETER_LINK, &ptr->link);
						if(ptr->link)
							netif_set_link_up(ptr->netifPtr);
						else
							netif_set_link_down(ptr->netifPtr);
					}
					ptr->arpController->voidCallback(this, NULL);
					ptr = ptr->next;
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
	if(this->firstInterface_ == NULL){
		if(this->routeFramesInTimer_){
			TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
					100000, this, (void*)CALLBACK_TYPE_ROUTE_FRAMES);
		}
		return;
	}
	EthernetRouterIface_t* ptr = this->firstInterface_;
	while(ptr != NULL){
		this->inputFrameSize_ = ptr->interface->pullOutRxFrame(&this->inputFrame_);
		if(this->inputFrameSize_) {
			uint8_t* frameU8 = (uint8_t*)&this->inputFrame_;
			if(EthernetUtilities::getFrameType(frameU8) == ETX_ETHER_TYPE_ARP){
				ptr->arpController->parseFrame(&this->inputFrame_, this->inputFrameSize_,
						ptr->interface, NULL);
				if(memcmp(&frameU8[ETX_ARP_TARGET_IP_OFFSET], ptr->ip, ETX_PROTO_SIZE) == 0){
					this->pushFrameToLwip(ptr->netifPtr);
				}
			}
			else{
				if(memcmp(&frameU8[ETX_ETH_D_MAC_OFFSET], ptr->mac, ETX_HW_SIZE) == 0){
					if(memcmp(&frameU8[ETX_IP_DIP_OFFSET], ptr->ip, ETX_PROTO_SIZE) == 0){
						this->pushFrameToLwip(ptr->netifPtr);
					}
					for(uint32_t i = 0; i < ETHERNET_ROUTER_MAX_NUM_LISTENERS; i++ ){
						if(ptr->listeners[i] == NULL)
							break;
						else{
							ptr->listeners[i]->parseFrame(&this->inputFrame_, this->inputFrameSize_,
									ptr->interface, ptr);
						}
					}
				}
			}
		}
		ptr = ptr->next;
	}
	__disable_irq();
	sys_check_timeouts();
	__enable_irq();
	if(this->routeFramesInTimer_){
		TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
				this->parsePeriodUs_, this, (void*)CALLBACK_TYPE_ROUTE_FRAMES);
	}
}



void EthernetRouter::pushFrameToLwip(struct netif* netifPtr)
{
	__disable_irq();
	lwip_ethernetif_input(netifPtr, &this->inputFrame_, this->inputFrameSize_);
	__enable_irq();
}



void EthernetRouter::addListener(IEthernetListener* listener,
		EthernetRouterIface_t* routerInterface)
{
	if(routerInterface){
		for(uint32_t i = 0; i < ETHERNET_ROUTER_MAX_NUM_LISTENERS; i++) {
			if(routerInterface->listeners[i] == listener)
				break;
			else if(routerInterface->listeners[i] == NULL){
				routerInterface->listeners[i] = listener;
				break;
			}
		}
	}
}



void EthernetRouter::addListener(IEthernetListener* listener, IVoidEthernet* interface)
{
	if(this->firstInterface_ == NULL)
		return;
	EthernetRouterIface_t* ptr = this->firstInterface_;
	while(ptr != NULL) {
		if(ptr->interface == interface){
			this->addListener(listener, ptr);
			break;
		}
		ptr = ptr->next;
	}
}



void EthernetRouter::addListener(IEthernetListener* listener)
{
	if(this->firstInterface_ == NULL)
		return;
	EthernetRouterIface_t* ptr = this->firstInterface_;
	while(ptr != NULL){
		this->addListener(listener, ptr);
		ptr = ptr->next;
	}
}



void EthernetRouter::deleteListener(IEthernetListener* listener,
		EthernetRouterIface_t* routerInterface)
{
	uint32_t index = 0;
	for(uint32_t i = 0; i < ETHERNET_ROUTER_MAX_NUM_LISTENERS; i++){
		if(routerInterface->listeners[i] == listener)
			break;
		else index++;
	}
	if(index == (ETHERNET_ROUTER_MAX_NUM_LISTENERS - 1)) {
		if(routerInterface->listeners[index] == listener)
			routerInterface->listeners[index] = NULL;
	}
	else{
		for(uint8_t i = index; i < (ETHERNET_ROUTER_MAX_NUM_LISTENERS - 1); i++) {
			routerInterface->listeners[i] = routerInterface->listeners[i + 1];
			if(routerInterface->listeners[i + 1] == NULL)
				break;
		}
	}
}



void EthernetRouter::deleteListener(IEthernetListener* listener,
		IVoidEthernet* interface)
{
	if(this->firstInterface_ == NULL)
		return;
	EthernetRouterIface_t* ptr = this->firstInterface_;
	while(ptr != NULL){
		if(ptr->interface == interface){
			this->deleteListener(listener, ptr);
			break;
		}
		ptr = ptr->next;
	}
}



void EthernetRouter::deleteListener(IEthernetListener* listener)
{
	if(this->firstInterface_ == NULL)
		return;
	EthernetRouterIface_t* ptr = this->firstInterface_;
	while(ptr != NULL) {
		this->deleteListener(listener, ptr);
		ptr = ptr->next;
	}
}

}
}

#endif
