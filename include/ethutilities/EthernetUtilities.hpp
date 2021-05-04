/**
 * @file
 * @brief Various Ethernet utilities class Description
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

#ifndef ETHERNET_UTILITIES_HPP_
#define ETHERNET_UTILITIES_HPP_

#include <stdint.h>

#define _HI(x)		(((x)>>8)&0xff)
#define _LO(x)		((x)&0xff)

#define ETX_HW_SIZE 	(6)
#define ETX_PROTO_SIZE 	(4)

#define ETX_ETHER_TYPE_IP 	((uint16_t)0x0800)
#define ETX_ETHER_TYPE_ARP 	((uint16_t)0x0806)

#define ETX_ETH_HEADER_LEN 			(14)
#define ETX_ETH_D_MAC_OFFSET 		(0)
#define ETX_ETH_S_MAC_OFFSET 		(6)
#define ETX_ETH_ETHER_TYPE_OFFSET 	(12)
#define ETX_ETH_CHECKSUM_SIZE		(4)

#define ETX_ARP_BODY_LEN (28)
#define ETX_ARP_HW_TYPE_OFFSET 		(ETX_ETH_HEADER_LEN + 0)
#define ETX_ARP_PROTO_TYPE_OFFSET 	(ETX_ETH_HEADER_LEN + 2)
#define ETX_ARP_HW_SIZE_OFFSET 		(ETX_ETH_HEADER_LEN + 4)
#define ETX_ARP_PROTO_SIZE_OFFSET 	(ETX_ETH_HEADER_LEN + 5)
#define ETX_ARP_OPCODE_OFFSET 		(ETX_ETH_HEADER_LEN + 6)
#define ETX_ARP_SENDER_MAC_OFFSET	(ETX_ETH_HEADER_LEN + 8)
#define ETX_ARP_SENDER_IP_OFFSET 	(ETX_ETH_HEADER_LEN + 14)
#define ETX_ARP_TARGET_MAC_OFFSET	(ETX_ETH_HEADER_LEN + 18)
#define ETX_ARP_TARGET_IP_OFFSET 	(ETX_ETH_HEADER_LEN + 24)

#define ETX_IP_HEADER_LEN		(20)
#define ETX_IP_HEADER_OFFSET	(ETX_ETH_HEADER_LEN)

#define ETX_IP_IHL_OFFSET					(ETX_IP_HEADER_OFFSET + 0)
#define ETX_IP_TOS_OFFSET 					(ETX_IP_HEADER_OFFSET + 1)
#define ETX_IP_TOTAL_LEN_OFFSET 		   	(ETX_IP_HEADER_OFFSET + 2)
#define ETX_IP_ID_OFFSET 				    (ETX_IP_HEADER_OFFSET + 4)
#define ETX_IP_FLAG_FRAGOFFS_OFFSET			(ETX_IP_HEADER_OFFSET + 6)
#define ETX_IP_TTL_OFFSET					(ETX_IP_HEADER_OFFSET + 8)
#define ETX_IP_PROTO_OFFSET 				(ETX_IP_HEADER_OFFSET + 9)
#define ETX_IP_CHECKSUM_OFFSET				(ETX_IP_HEADER_OFFSET + 10)
#define ETX_IP_SIP_OFFSET 					(ETX_IP_HEADER_OFFSET + 12)
#define ETX_IP_DIP_OFFSET 					(ETX_IP_HEADER_OFFSET + 16)
#define ETX_IP_PARAM_OFFSET 				(ETX_IP_HEADER_OFFSET + 20)
#define ETX_IP_DATA_OFFSET 					(ETX_IP_HEADER_OFFSET + ETX_IP_HEADER_LEN)


#define ETX_IPH_IHL_DEF_VAL 			(0x45)
#define ETX_IPH_TOS_DEF_VAL 			(0x00)
#define ETX_IPH_FLAG_FRAGOFFS_DEF_VAL 	(0x00)
#define ETX_IPH_TTL_DEF_VAL 			(128)
#define ETX_IPH_PROTO_UDP				(0x11)

#define ETX_UDP_HEADER_LEN         	(8)
#define ETX_UDP_HEADER_OFFSET		(ETX_ETH_HEADER_LEN + ETX_IP_HEADER_LEN)
#define ETX_UDP_SPORT_OFFSET		(ETX_UDP_HEADER_OFFSET + 0)
#define ETX_UDP_DPORT_OFFSET		(ETX_UDP_HEADER_OFFSET + 2)
#define ETX_UDP_LEN_OFFSET			(ETX_UDP_HEADER_OFFSET + 4)
#define ETX_UDP_CHECKSUM_OFFSET		(ETX_UDP_HEADER_OFFSET + 6)
#define ETX_UDP_DATA_OFFSET			(ETX_UDP_HEADER_OFFSET + ETX_UDP_HEADER_LEN)

#define ETX_UDPPSH_PSEUDOHEADER_LEN      (12)
#define ETX_UDPPSH_SIP_OFFSET            (0)
#define ETX_UDPPSH_DIP_OFFSET            (4)
#define ETX_UDPPSH_NULL_OFFSET           (8)
#define ETX_UDPPSH_PROTO_OFFSET          (9)
#define ETX_UDPPSH_UDP_LEN_OFFSET        (10)

namespace jblib
{
namespace ethutilities
{

class EthernetUtilities
{
public:
	static uint16_t getFrameType(uint8_t* frame);
	static void createIpHeader(uint8_t* frame, uint8_t* dstIp,
			uint8_t* srcIp, uint32_t dataSize, uint8_t* srcMac,
			uint8_t* dstMac, uint8_t protocol);
	static bool createUdpHeader(uint8_t* frame, uint16_t* headerSize,
			uint8_t* dstIp, uint16_t dstPort, uint8_t* srcIp, uint16_t srcPort,
			uint32_t dataSize, uint8_t* srcMac, uint8_t* dstMac);
	static bool createUdpFrame(uint8_t* frame, uint16_t* frameSize,
			uint8_t* dstIp, uint16_t dstPort, uint8_t* srcIp,
			uint16_t srcPort, uint8_t* udpData, uint32_t dataSize,
			uint8_t* srcMac, uint8_t* dstMac, uint8_t ECN);
	static uint8_t* getUdpData(uint8_t* frame, uint16_t* dataSize);
	static void changeIp(uint8_t* frame, uint8_t* newDstIp, uint8_t* newSrcIp);
	static void maskIp(uint8_t* ip, uint8_t* mask);
	static int compareIp(uint8_t* ip1, uint8_t* ip2, uint8_t* mask);
	static uint16_t calculateNetworkChecksum(uint8_t* data, uint16_t dataSize,
			uint16_t startValue);

private:
	static void adjustNetworkChecksum(uint8_t* chksum, uint8_t* optr,
			uint16_t olen, uint8_t* nptr, uint16_t nlen);

	static uint16_t ipId_;
};

}
}

#endif /* ETHERNET_UTILITIES_HPP_ */
