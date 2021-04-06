/**
 * @file
 * @brief Arp Controller class Description
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

#ifndef ARP_CONTROLLER_HPP_
#define ARP_CONTROLLER_HPP_

#include "jbkernel/jb_common.h"
#include "ethutilities/EthernetUtilities.hpp"
#include "jbkernel/IVoidEthernet.hpp"
#include "jbkernel/callback_interfaces.hpp"

namespace jblib
{
namespace ethutilities
{

using namespace jbkernel;

#pragma pack(push, 1)
typedef struct {
	uint8_t ip[ARP_CONTROLLER_MAX_NUM_IP_FOR_REPLY][ETX_PROTO_SIZE];
	uint8_t mask[ARP_CONTROLLER_MAX_NUM_IP_FOR_REPLY][ETX_PROTO_SIZE];
	uint16_t ipCount = 0;
}IpTableForReply_t;



typedef struct{
	uint8_t ip[ETX_PROTO_SIZE]; 		// ip
	uint8_t mac[ETX_HW_SIZE]; 			//mac
	uint32_t timeAfterUpdate = 0;		//time after last arp update
}ArpTableLine_t;



typedef struct {
	uint16_t recordCounter = 0;
	ArpTableLine_t line[ARP_CONTROLLER_ARP_TABLE_SIZE];
}ArpTable_t;
#pragma pack(pop)



class ArpController : public IEthernetListener, IVoidCallback
{
public:
	ArpController(IVoidEthernet* ethernetAdapter);
	virtual ~ArpController(void);
	virtual void parseFrame(EthernetFrame* const frame,
				uint16_t frameSize, IVoidEthernet* const source, void* parameter);
	void sendGratiousArp(uint8_t* srcIp);
	void sendArpRequest(uint8_t* dstIp);
	void sendArpRequest(uint8_t* dstIp, uint8_t* srcIp);
	void addIpForArpReply(uint8_t* ip);
	void addIpForArpReply(uint8_t* ip, uint8_t* mask);
	bool isIpInTableForReply(uint8_t* ip);
	bool getMac(uint8_t* ip, uint8_t* mac);
	bool getIp(uint8_t* mac, uint8_t* ip);
	uint32_t getTimeAfterUpdate(uint8_t* ip);
	virtual void voidCallback(void* const source, void* parameter);
	ArpTable_t* getArpTable(void);

private:
	int16_t getArpTableIndex(uint8_t* ip);
	void createReply(uint8_t* request, uint8_t* reply, uint8_t* srcMac, uint16_t* frameSize);
	void createReply(const uint8_t* dstMac, const uint8_t* dstIp, uint8_t* srcMac, uint8_t* srcIp, uint8_t* reply, uint16_t* frameSize);
	void createRequest(const uint8_t* dstIp, uint8_t* srcMac,uint8_t* srcIp, uint8_t* request, uint16_t* frameSize);

	uint8_t* mac_ = NULL;
	IVoidEthernet* ethernetAdapter_ = NULL;
	IpTableForReply_t ipTableForReply_;
	ArpTable_t arpTable_;
};

}
}

#endif /* ARP_CONTROLLER_HPP_ */
