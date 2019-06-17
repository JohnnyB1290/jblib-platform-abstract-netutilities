/**
 * @file
 * @brief Arp Controller class Realization
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

#include <string.h>
#include "ethutilities/ArpController.hpp"
#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
#include <stdio.h>
#endif

namespace jblib
{
namespace ethutilities
{

using namespace jbkernel;



ArpController::ArpController(IVoidEthernet* ethernetAdapter) :
		IEthernetListener(), IVoidCallback()
{
	this->ethernetAdapter_ = ethernetAdapter;
	this->ethernetAdapter_->getParameter(PARAMETER_MAC, (void*)&(this->mac_));
	memset(&(this->ipTableForReply_), 0, sizeof(IpTableForReply_t));
	memset(&(this->arpTable_), 0, sizeof(ArpTable_t));
}



ArpController::~ArpController(void)
{

}



ArpTable_t* ArpController::getArpTable(void)
{
	return &this->arpTable_;
}



void ArpController::createReply(uint8_t* request, uint8_t* reply,
		uint8_t* srcMac, uint16_t* frameSize)
{
		*frameSize = ETX_ETH_HEADER_LEN + ETX_ARP_BODY_LEN;
		memcpy(reply + ETX_ETH_D_MAC_OFFSET,
				request + ETX_ARP_SENDER_MAC_OFFSET, ETX_HW_SIZE);
		memcpy(reply + ETX_ETH_S_MAC_OFFSET, srcMac, ETX_HW_SIZE);
		reply[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
		reply[ETX_ETH_ETHER_TYPE_OFFSET + 1] = 0x06; // ARP
		reply[ETX_ARP_HW_TYPE_OFFSET] = 0x00;
		reply[ETX_ARP_HW_TYPE_OFFSET + 1] = 0x01; // ETHERNET
		reply[ETX_ARP_PROTO_TYPE_OFFSET] = 0x08;
		reply[ETX_ARP_PROTO_TYPE_OFFSET + 1] = 0x00;//ip
		reply[ETX_ARP_HW_SIZE_OFFSET] = 0x06;
		reply[ETX_ARP_PROTO_SIZE_OFFSET] = 0x04;
		reply[ETX_ARP_OPCODE_OFFSET] = 0x00;//reply
		reply[ETX_ARP_OPCODE_OFFSET + 1] = 0x02;//reply
		memcpy(reply + ETX_ARP_SENDER_MAC_OFFSET, srcMac, ETX_HW_SIZE);
		memcpy(reply + ETX_ARP_SENDER_IP_OFFSET,
				request + ETX_ARP_TARGET_IP_OFFSET, ETX_PROTO_SIZE);
		memcpy(reply + ETX_ARP_TARGET_MAC_OFFSET,
				request + ETX_ARP_SENDER_MAC_OFFSET, ETX_HW_SIZE);
		memcpy(reply + ETX_ARP_TARGET_IP_OFFSET,
				request + ETX_ARP_SENDER_IP_OFFSET, ETX_PROTO_SIZE);
}



void ArpController::createRequest(const uint8_t* dstIp, uint8_t* srcMac,uint8_t* srcIp,
		uint8_t* request, uint16_t* frameSize)
{
		*frameSize = ETX_ETH_HEADER_LEN + ETX_ARP_BODY_LEN;
		memset(request + ETX_ETH_D_MAC_OFFSET, 0xFF, ETX_HW_SIZE);
		memcpy(request + ETX_ETH_S_MAC_OFFSET, srcMac, ETX_HW_SIZE);
		request[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
		request[ETX_ETH_ETHER_TYPE_OFFSET + 1] = 0x06; // ARP
		request[ETX_ARP_HW_TYPE_OFFSET] = 0x00;
		request[ETX_ARP_HW_TYPE_OFFSET + 1] = 0x01; // ETHERNET
		request[ETX_ARP_PROTO_TYPE_OFFSET] = 0x08;
		request[ETX_ARP_PROTO_TYPE_OFFSET + 1] = 0x00;//dstIp
		request[ETX_ARP_HW_SIZE_OFFSET] = 0x06;
		request[ETX_ARP_PROTO_SIZE_OFFSET] = 0x04;
		request[ETX_ARP_OPCODE_OFFSET] = 0x00;//
		request[ETX_ARP_OPCODE_OFFSET + 1] = 0x01;//request
		memcpy(request + ETX_ARP_SENDER_MAC_OFFSET, srcMac, ETX_HW_SIZE);
		memcpy(request + ETX_ARP_SENDER_IP_OFFSET, srcIp, ETX_PROTO_SIZE);
		memset(request + ETX_ARP_TARGET_MAC_OFFSET, 0, ETX_HW_SIZE);
		memcpy(request + ETX_ARP_TARGET_IP_OFFSET, dstIp, ETX_PROTO_SIZE);
}



void ArpController::parseFrame(EthernetFrame* const frame,
		uint16_t frameSize, IVoidEthernet* const source, void* parameter)
{
	static uint8_t* outputFrame = NULL;
	static uint16_t outputFrameSize = 0;
	static uint8_t* inputFrame = NULL;
	
	if(frameSize == 0)
		return;
	inputFrame = (uint8_t*)frame;
	if(EthernetUtilities::getFrameType(inputFrame) != ETX_ETHER_TYPE_ARP)
		return;

	bool allowArpFlag = false;
	if(parameter == NULL) {
		if(this->isIpInTableForReply(&(inputFrame[ETX_ARP_TARGET_IP_OFFSET])))
			allowArpFlag = true;
		else
			allowArpFlag = false;
	}
	else
		allowArpFlag = true;

	if(allowArpFlag) {
		if((inputFrame[ETX_ARP_OPCODE_OFFSET] == 0x00) &&
				(inputFrame[ETX_ARP_OPCODE_OFFSET+1] == 0x02)) { //reply

			#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
			char* adapterName;
			this->ethernetAdapter_->getParameter(PARAMETER_NAME, (void*)&adapterName);
			#endif
			int16_t index = this->getArpTableIndex(&(inputFrame[ETX_ARP_SENDER_IP_OFFSET]));
			if(index >= 0) {
				memcpy(this->arpTable_.line[index].mac,
						&(inputFrame[ETX_ARP_SENDER_MAC_OFFSET]), ETX_HW_SIZE);
				this->arpTable_.line[index].timeAfterUpdate = 0;
			}
			else {
				index = this->arpTable_.recordCounter;
				memcpy(this->arpTable_.line[index].mac,
						&(inputFrame[ETX_ARP_SENDER_MAC_OFFSET]), ETX_HW_SIZE);
				memcpy(this->arpTable_.line[index].ip,
						&(inputFrame[ETX_ARP_SENDER_IP_OFFSET]), ETX_PROTO_SIZE);
				this->arpTable_.line[index].timeAfterUpdate = 0;
				this->arpTable_.recordCounter++;
				if(this->arpTable_.recordCounter == ARP_CONTROLLER_ARP_TABLE_SIZE)
					this->arpTable_.recordCounter = 0;
				#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
				printf("Create Arp record\r\n");
				#endif
			}
			#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
			printf("%s Arp reply FROM %i.%i.%i.%i ",
					adapterName,
					this->arpTable_.line[index].ip[0],
					this->arpTable_.line[index].ip[1],
					this->arpTable_.line[index].ip[2],
					this->arpTable_.line[index].ip[3]);
			printf("FOR %i.%i.%i.%i\r\n",
					inputFrame[ETX_ARP_TARGET_IP_OFFSET + 0],
					inputFrame[ETX_ARP_TARGET_IP_OFFSET + 1],
					inputFrame[ETX_ARP_TARGET_IP_OFFSET + 2],
					inputFrame[ETX_ARP_TARGET_IP_OFFSET + 3]);
			#endif
		}
		else if((inputFrame[ETX_ARP_OPCODE_OFFSET] == 0x00) &&
				(inputFrame[ETX_ARP_OPCODE_OFFSET+1] == 0x01)) { //request

			#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
			char* adapterName;
			this->ethernetAdapter_->getParameter(PARAMETER_NAME, (void*)&adapterName);
			#endif
			int16_t index = this->getArpTableIndex(&inputFrame[ETX_ARP_SENDER_IP_OFFSET]);
			if(index >= 0){
				memcpy(this->arpTable_.line[index].mac,
						&(inputFrame[ETX_ARP_SENDER_MAC_OFFSET]), ETX_HW_SIZE);
				this->arpTable_.line[index].timeAfterUpdate = 0;
			}
			else {
				index = this->arpTable_.recordCounter;
				memcpy(this->arpTable_.line[index].mac,
						&inputFrame[ETX_ARP_SENDER_MAC_OFFSET], ETX_HW_SIZE);
				memcpy(this->arpTable_.line[index].ip,
						&(inputFrame[ETX_ARP_SENDER_IP_OFFSET]),ETX_PROTO_SIZE);
				this->arpTable_.line[index].timeAfterUpdate = 0;
				this->arpTable_.recordCounter++;
				if(this->arpTable_.recordCounter == ARP_CONTROLLER_ARP_TABLE_SIZE)
					this->arpTable_.recordCounter = 0;
				#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
				printf("Create Arp record\r\n");
				#endif
			}
			#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
			printf("%s Arp request FROM %i.%i.%i.%i ",
					adapterName,
					this->arpTable_.line[index].ip[0],
					this->arpTable_.line[index].ip[1],
					this->arpTable_.line[index].ip[2],
					this->arpTable_.line[index].ip[3]);
			printf("FOR %i.%i.%i.%i\r\n",
					inputFrame[ETX_ARP_TARGET_IP_OFFSET + 0],
					inputFrame[ETX_ARP_TARGET_IP_OFFSET + 1],
					inputFrame[ETX_ARP_TARGET_IP_OFFSET + 2],
					inputFrame[ETX_ARP_TARGET_IP_OFFSET + 3]);
			#endif
			outputFrame = (uint8_t*)malloc_s(sizeof(EthernetFrame));
			if(outputFrame){
				this->createReply(inputFrame, outputFrame, this->mac_, &outputFrameSize);
				this->ethernetAdapter_->addToTxQueue((EthernetFrame*)outputFrame,
						outputFrameSize);
				free_s(outputFrame);
				#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
				printf("Send reply.\r\n");
				#endif
			}
		}
	}
}



void ArpController::sendArpRequest(uint8_t* dstIp)
{
	this->sendArpRequest(dstIp, NULL);
}



void ArpController::sendArpRequest(uint8_t* dstIp, uint8_t* srcIp)
{
	uint8_t* outputFrame = (uint8_t*)malloc_s(sizeof(EthernetFrame));
	if(outputFrame == NULL)
		return;
	uint16_t outputFrameSize = 0;

	#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
	char* adapterName;
	this->ethernetAdapter_->getParameter(PARAMETER_NAME, (void*)&adapterName);
	#endif
	if(srcIp == NULL){
		for(uint32_t i = 0; i < this->ipTableForReply_.ipCount; i++) {
			if(EthernetUtilities::compareIp(dstIp, this->ipTableForReply_.ip[i],
					this->ipTableForReply_.mask[i]) == 0){
				this->createRequest(dstIp, this->mac_,
						this->ipTableForReply_.ip[i], outputFrame, &outputFrameSize);
				this->ethernetAdapter_->addToTxQueue((EthernetFrame*)outputFrame,
						outputFrameSize);
				#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
				printf("%s Send ARP request FOR %i.%i.%i.%i ",
						adapterName, dstIp[0], dstIp[1], dstIp[2],dstIp[3]);
				printf("FROM %i.%i.%i.%i\r\n",
						this->ipTableForReply_.ip[i][0],
						this->ipTableForReply_.ip[i][1],
						this->ipTableForReply_.ip[i][2],
						this->ipTableForReply_.ip[i][3]);
				#endif
			}
		}
	}
	else {
		this->createRequest(dstIp, this->mac_, srcIp, outputFrame, &outputFrameSize);
		this->ethernetAdapter_->addToTxQueue((EthernetFrame*)outputFrame, outputFrameSize);
		#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
		printf("%s Send ARP request FOR %i.%i.%i.%i ",
				adapterName, dstIp[0], dstIp[1], dstIp[2],dstIp[3]);
		printf("FROM %i.%i.%i.%i\r\n",
				srcIp[0],srcIp[1], srcIp[2], srcIp[3]);
		#endif
	}
	free_s(outputFrame);
}



void ArpController::addIpForArpReply(uint8_t* ip)
{
	uint8_t mask[ETX_PROTO_SIZE] = {255, 255, 255, 0};
	this->addIpForArpReply(ip, mask);
}



void ArpController::addIpForArpReply(uint8_t* ip, uint8_t* mask)
{
	memcpy(this->ipTableForReply_.ip[this->ipTableForReply_.ipCount], ip, ETX_PROTO_SIZE);
	memcpy(this->ipTableForReply_.mask[this->ipTableForReply_.ipCount], mask, ETX_PROTO_SIZE);
	this->ipTableForReply_.ipCount++;
	if(this->ipTableForReply_.ipCount == ARP_CONTROLLER_MAX_NUM_IP_FOR_REPLY)
		this->ipTableForReply_.ipCount = 0;
}



bool ArpController::isIpInTableForReply(uint8_t* ip)
{
	if(this->ipTableForReply_.ipCount == 0)
		return false;
	for(uint32_t i=0; i < (this->ipTableForReply_.ipCount); i++) {
		if((memcmp(ip, this->ipTableForReply_.ip[i], ETX_PROTO_SIZE) == 0))
			return true;
	}
	return false;
}



int16_t ArpController::getArpTableIndex(uint8_t* ip)
{
	if(this->arpTable_.recordCounter == 0)
		return -1;
	for(int32_t index = 0; index < this->arpTable_.recordCounter; index++) {
		if(memcmp(ip, this->arpTable_.line[index].ip, ETX_PROTO_SIZE)==0)
			return index;
	}
	return -1;
}



bool ArpController::getMac(uint8_t* ip, uint8_t* mac)
{
	int16_t index = this->getArpTableIndex(ip);
	if(index >= 0) {
		memcpy(mac, this->arpTable_.line[index].mac, ETX_HW_SIZE);
		return true;
	}
	else
		return false;
}



bool ArpController::getIp(uint8_t* mac, uint8_t* ip)
{
	if(this->arpTable_.recordCounter == 0)
		return false;
	for(uint32_t index = 0; index < this->arpTable_.recordCounter; index++) {
		if(memcmp(mac, this->arpTable_.line[index].mac, ETX_HW_SIZE)==0) {
			memcpy(ip, this->arpTable_.line[index].ip, ETX_PROTO_SIZE);
			return true;
		}
	}
	return false;
}



void ArpController::voidCallback(void* const source, void* parameter)
{
	if(this->arpTable_.recordCounter > 0) {
		for(int i = 0; i < this->arpTable_.recordCounter; i++) {
			this->arpTable_.line[i].timeAfterUpdate++;
			if(this->arpTable_.line[i].timeAfterUpdate > ARP_CONTROLLER_REFRESH_RECORD_TIME)
				this->sendArpRequest(this->arpTable_.line[i].ip);
			if(arpTable_.line[i].timeAfterUpdate > ARP_CONTROLLER_DELETE_RECORD_TIME) {
				#if (USE_CONSOLE && ARP_CONTROLLER_USE_CONSOLE)
				char* adapterName;
				this->ethernetAdapter_->getParameter(PARAMETER_NAME, (void*)&adapterName);
				printf("%s Arp table delete record for ip %i.%i.%i.%i\r\n",
						adapterName,
						this->arpTable_.line[i].ip[0],
						this->arpTable_.line[i].ip[1],
						this->arpTable_.line[i].ip[2],
						this->arpTable_.line[i].ip[3]);
				#endif
				if(i != (this->arpTable_.recordCounter-1)) {
					memmove(&this->arpTable_.line[i],
							&this->arpTable_.line[i+1],
							sizeof(ArpTableLine_t) *
							(this->arpTable_.recordCounter - 1 - i));
				}
				this->arpTable_.recordCounter--;
			}
		}
	}
}

}
}
