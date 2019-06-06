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

#include "arch/systick_arch.h"
#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "arch/ethernetif.h"
#include "JbController.hpp"
#include "EthernetRouter.hpp"
#include "TimeEngine.hpp"
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
	for(uint32_t i = 0; i < ETHERNET_ROUTER_MAX_NUM_INTERFACES; i++){
		this->interfaces_[i] = NULL;
		this->arpControllers_[i] = NULL;
		for(uint32_t j = 0; j < ETHERNET_ROUTER_MAX_NUM_LISTENERS; j++)
			this->listeners_[i][j] = (IEthernetListener*)NULL;
	}
	#if (USE_CONSOLE && ETHERNET_ROUTER_USE_CONSOLE)
	printf("Ethernet router: Starting LWIP Library\r\n");
	#endif
	lwip_init();
}



void EthernetRouter::setParsePeriodUs(uint32_t parsePeriodUs)
{
	this->parsePeriodUs_ = parsePeriodUs;
}



void EthernetRouter::start(uint8_t nrtTimerNumber, bool routeFramesInTimer)
{
	static bool isStarted = false;
	if(!isStarted){
		this->nrtTimerNumber_ = nrtTimerNumber;
		this->routeFramesInTimer_ = routeFramesInTimer;
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



struct netif* EthernetRouter::getNetif(IVoidEthernet* interface)
{
	for(uint32_t i=0; i < ETHERNET_ROUTER_MAX_NUM_INTERFACES; i++) {
		if(this->interfaces_[i] == interface)
			return &this->netifs_[i];
	}
	return (struct netif*)NULL;
}



void EthernetRouter::addInterface(IVoidEthernet* interface, uint8_t* ip,
		uint8_t* gateway, uint8_t* netmask)
{
	if(this->interfacesCounter_ == ETHERNET_ROUTER_MAX_NUM_INTERFACES){
		#if (USE_CONSOLE && ETHERNET_ROUTER_USE_CONSOLE)
		printf("Ethernet router: Can't add interface, max number of interfaces\r\n");
		#endif
		return;
	}
	this->interfaces_[this->interfacesCounter_] = interface;
	interface->initialize();
	interface->start();
	this->arpControllers_[this->interfacesCounter_] = new ArpController(interface);
	this->arpControllers_[this->interfacesCounter_]->addIpForArpReply(ip);
	ip_addr_t ipaddr, mask, gw;
	IP4_ADDR(&ipaddr, ip[0], ip[1], ip[2], ip[3]);
	IP4_ADDR(&gw, gateway[0], gateway[1], gateway[2], gateway[3]);
	IP4_ADDR(&mask, netmask[0], netmask[1], netmask[2], netmask[3]);

	#if (USE_CONSOLE && ETHERNET_ROUTER_USE_CONSOLE)
	char* adapterName = NULL;
	interface->getParameter(PARAMETER_NAME, (void*)&adapterName);
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
	netif_add(&this->netifs_[this->interfacesCounter_], &ipaddr, &mask, &gw,
			(void*)interface, lwip_ethernetif_init, ethernet_input);
//	if(this->interfacesCounter_ == 0)
//		netif_set_default(&this->netifs_[this->interfacesCounter_]);
	netif_set_up(&this->netifs_[this->interfacesCounter_]);
	this->interfacesCounter_++;
	this->addListener(this->arpControllers_[this->interfacesCounter_ - 1], interface);
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
				for(uint32_t i = 0; i < this->interfacesCounter_; i++) {
					bool link = 0;
					this->interfaces_[i]->getParameter(PARAMETER_LINK, &link);
					if(link)
						netif_set_link_up(&this->netifs_[i]);
					else
						netif_set_link_down(&this->netifs_[i]);

					this->arpControllers_[i]->voidCallback(this, NULL);
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
	if(this->interfacesCounter_ == 0) {
		if(this->routeFramesInTimer_){
			TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
					100000, this, (void*)CALLBACK_TYPE_ROUTE_FRAMES);
		}
		return;
	}
	for(uint32_t i = 0; i < this->interfacesCounter_; i++) {
		this->inputFrameSize_ =
				this->interfaces_[i]->pullOutRxFrame(&this->inputFrame_);
		if(this->inputFrameSize_) {
			__disable_irq();
			lwip_ethernetif_input(&this->netifs_[i],
					&this->inputFrame_, this->inputFrameSize_);
			__enable_irq();
			for(uint32_t j = 0; j < ETHERNET_ROUTER_MAX_NUM_LISTENERS; j++ ) {
				if(this->listeners_[i][j] == (IEthernetListener*)NULL)
					break;
				else
					this->listeners_[i][j]->parseFrame(&this->inputFrame_,
							this->inputFrameSize_, this->interfaces_[i], NULL);
			}
		}
	}
	__disable_irq();
	sys_check_timeouts();
	__enable_irq();
	if(this->routeFramesInTimer_){
		TimeEngine::getTimeEngine()->setNrtEvent(this->nrtTimerNumber_,
				this->parsePeriodUs_, this, (void*)CALLBACK_TYPE_ROUTE_FRAMES);
	}
}



ArpController* EthernetRouter::getArpController(IVoidEthernet* interface)
{
	if(this->interfacesCounter_ == 0)
		return (ArpController*)NULL;
	for(uint32_t i = 0; i<this->interfacesCounter_; i++) {
		if(this->interfaces_[i] == (IVoidEthernet*)interface)
			return this->arpControllers_[i];
	}
	return NULL;
}



void EthernetRouter::addListener(IEthernetListener* listener, IVoidEthernet* interface)
{
	if(this->interfacesCounter_ == 0)
		return;
	for(uint32_t i = 0; i < this->interfacesCounter_; i++) {
		if(this->interfaces_[i] == (IVoidEthernet*)interface) {
			for(uint32_t j = 0; j < ETHERNET_ROUTER_MAX_NUM_LISTENERS; j++) {
				if(this->listeners_[i][j] == listener)
					break;
				if(this->listeners_[i][j] == (IEthernetListener*)NULL) {
					this->listeners_[i][j] = listener;
					break;
				}
			}
			break;
		}
	}
}



void EthernetRouter::addListener(IEthernetListener* listener)
{
	for(uint32_t i = 0; i < this->interfacesCounter_; i++)
		this->addListener(listener, this->interfaces_[i]);
}



void EthernetRouter::deleteListener(IEthernetListener* listener, IVoidEthernet* interface)
{
	if(this->interfacesCounter_ == 0)
		return;
	for(uint32_t i = 0; i < this->interfacesCounter_; i++) {
		if(this->interfaces_[i] == (IVoidEthernet*)interface) {
			uint32_t index = 0;
			for(uint8_t j = 0; j < ETHERNET_ROUTER_MAX_NUM_LISTENERS; j++) {
				if(this->listeners_[i][j] == listener)
					break;
				else
					index++;
			}
			if(index == (ETHERNET_ROUTER_MAX_NUM_LISTENERS-1)) {
				if(this->listeners_[i][index] == listener)
					this->listeners_[i][index] = (IEthernetListener*)NULL;
			}
			else {
				for(uint32_t j = index; j < (ETHERNET_ROUTER_MAX_NUM_LISTENERS - 1); j++) {
					this->listeners_[i][j] = this->listeners_[i][j+1];
					if(this->listeners_[i][j+1] == (IEthernetListener*)NULL)
						break;
				}
			}
			break;
		}
	}
}



void EthernetRouter::deleteListener(IEthernetListener* listener)
{
	for(uint32_t i = 0; i < this->interfacesCounter_; i++)
		this->deleteListener(listener,this->interfaces_[i]);
}

}
}
