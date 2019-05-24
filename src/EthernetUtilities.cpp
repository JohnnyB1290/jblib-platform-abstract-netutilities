/**
 * @file
 * @brief Various Ethernet utilities class Realization
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
#include "jb_common.h"
#include "EthernetUtilities.hpp"

namespace jblib::ethutilities
{

uint16_t EthernetUtilities::ipId_ = 0;



uint16_t EthernetUtilities::getFrameType(uint8_t* frame)
{
	 uint16_t retval = ((uint16_t)frame[ETX_ETH_ETHER_TYPE_OFFSET]<<8)|(frame[ETX_ETH_ETHER_TYPE_OFFSET+1]);
	 return retval;
}



void EthernetUtilities::createIpHeader(uint8_t* frame, uint8_t* dstIp,
		uint8_t* srcIp, uint32_t dataSize, uint8_t* srcMac,
		uint8_t* dstMac, uint8_t protocol)
{
	memcpy(frame + ETX_ETH_D_MAC_OFFSET, dstMac, ETX_HW_SIZE);
	memcpy(frame + ETX_ETH_S_MAC_OFFSET, srcMac, ETX_HW_SIZE);
	frame[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
	frame[ETX_ETH_ETHER_TYPE_OFFSET+1] = 0x00;//ip

	frame[ETX_IP_IHL_OFFSET] = ETX_IPH_IHL_DEF_VAL;
	frame[ETX_IP_TOS_OFFSET] = ETX_IPH_TOS_DEF_VAL;
	frame[ETX_IP_TOTAL_LEN_OFFSET] = _HI(ETX_IP_HEADER_LEN + dataSize);
	frame[ETX_IP_TOTAL_LEN_OFFSET+1] = _LO(ETX_IP_HEADER_LEN + dataSize);
	frame[ETX_IP_ID_OFFSET] = _HI(ipId_);
	frame[ETX_IP_ID_OFFSET+1] = _LO(ipId_);
	frame[ETX_IP_FLAG_FRAGOFFS_OFFSET] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;
	frame[ETX_IP_FLAG_FRAGOFFS_OFFSET+1] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;

	frame[ETX_IP_TTL_OFFSET] = ETX_IPH_TTL_DEF_VAL;
	frame[ETX_IP_PROTO_OFFSET] = protocol;
	frame[ETX_IP_CHECKSUM_OFFSET] = 0;
	frame[ETX_IP_CHECKSUM_OFFSET+1] = 0;

	memcpy(frame + ETX_IP_SIP_OFFSET, srcIp, ETX_PROTO_SIZE);
	memcpy(frame + ETX_IP_DIP_OFFSET, dstIp, ETX_PROTO_SIZE);

	uint16_t checksum = calculateNetworkChecksum(frame + ETX_IP_HEADER_OFFSET,
			ETX_IP_HEADER_LEN, 0);
	frame[ETX_IP_CHECKSUM_OFFSET] = _HI(checksum ^ 0xffff);
	frame[ETX_IP_CHECKSUM_OFFSET+1] = _LO(checksum ^ 0xffff);
	EthernetUtilities::ipId_++;
}



bool EthernetUtilities::createUdpHeader(uint8_t* frame, uint16_t* headerSize,
		uint8_t* dstIp, uint16_t dstPort, uint8_t* srcIp, uint16_t srcPort,
		uint32_t dataSize, uint8_t* srcMac, uint8_t* dstMac)
{
	*headerSize = ETX_UDP_DATA_OFFSET;
	if(dataSize > (EMAC_ETH_MAX_FLEN - ETX_UDP_DATA_OFFSET))
		return false;

	memcpy(frame + ETX_ETH_D_MAC_OFFSET, dstMac, ETX_HW_SIZE);
	memcpy(frame + ETX_ETH_S_MAC_OFFSET, srcMac, ETX_HW_SIZE);
	frame[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
	frame[ETX_ETH_ETHER_TYPE_OFFSET+1] = 0x00;//ip

	frame[ETX_IP_IHL_OFFSET] = ETX_IPH_IHL_DEF_VAL;
	frame[ETX_IP_TOS_OFFSET] = ETX_IPH_TOS_DEF_VAL;
	frame[ETX_IP_TOTAL_LEN_OFFSET] =
			_HI(ETX_IP_HEADER_LEN + ETX_UDP_HEADER_LEN + dataSize);
	frame[ETX_IP_TOTAL_LEN_OFFSET+1] =
			_LO(ETX_IP_HEADER_LEN + ETX_UDP_HEADER_LEN + dataSize);
	frame[ETX_IP_ID_OFFSET] = _HI(ipId_);
	frame[ETX_IP_ID_OFFSET+1] = _LO(ipId_);
	frame[ETX_IP_FLAG_FRAGOFFS_OFFSET] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;
	frame[ETX_IP_FLAG_FRAGOFFS_OFFSET+1] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;

	frame[ETX_IP_TTL_OFFSET] = ETX_IPH_TTL_DEF_VAL;
	frame[ETX_IP_PROTO_OFFSET] = ETX_IPH_PROTO_UDP;
	frame[ETX_IP_CHECKSUM_OFFSET] = 0;
	frame[ETX_IP_CHECKSUM_OFFSET+1] = 0;

	memcpy(frame + ETX_IP_SIP_OFFSET, srcIp, ETX_PROTO_SIZE);
	memcpy(frame + ETX_IP_DIP_OFFSET, dstIp, ETX_PROTO_SIZE);

	frame[ETX_UDP_SPORT_OFFSET] = _HI(srcPort);
	frame[ETX_UDP_SPORT_OFFSET+1] = _LO(srcPort);
	frame[ETX_UDP_DPORT_OFFSET] = _HI(dstPort);
	frame[ETX_UDP_DPORT_OFFSET+1] = _LO(dstPort);
	frame[ETX_UDP_LEN_OFFSET] = _HI(dataSize + ETX_UDP_HEADER_LEN);
	frame[ETX_UDP_LEN_OFFSET+1] = _LO(dataSize + ETX_UDP_HEADER_LEN);
	frame[ETX_UDP_CHECKSUM_OFFSET] = 0;
	frame[ETX_UDP_CHECKSUM_OFFSET+1] = 0;

	uint8_t pseudoheader[ETX_UDPPSH_PSEUDOHEADER_LEN];
	memcpy(pseudoheader + ETX_UDPPSH_SIP_OFFSET, srcIp, ETX_PROTO_SIZE);
	memcpy(pseudoheader + ETX_UDPPSH_DIP_OFFSET, dstIp, ETX_PROTO_SIZE);
	pseudoheader[ETX_UDPPSH_NULL_OFFSET] = 0;
	pseudoheader[ETX_UDPPSH_PROTO_OFFSET] = ETX_IPH_PROTO_UDP;
	pseudoheader[ETX_UDPPSH_UDP_LEN_OFFSET] = _HI(dataSize + ETX_UDP_HEADER_LEN);
	pseudoheader[ETX_UDPPSH_UDP_LEN_OFFSET+1] = _LO(dataSize + ETX_UDP_HEADER_LEN);

	uint16_t checksum = calculateNetworkChecksum(frame + ETX_UDP_HEADER_OFFSET,
			dataSize + ETX_UDP_HEADER_LEN,
			calculateNetworkChecksum(pseudoheader, ETX_UDPPSH_PSEUDOHEADER_LEN, 0));
	frame[ETX_UDP_CHECKSUM_OFFSET] = _HI(checksum ^ 0xffff);
	frame[ETX_UDP_CHECKSUM_OFFSET+1] = _LO(checksum ^ 0xffff);

	checksum = calculateNetworkChecksum(frame + ETX_IP_HEADER_OFFSET,
			ETX_IP_HEADER_LEN, 0);
	frame[ETX_IP_CHECKSUM_OFFSET] = _HI(checksum ^ 0xffff);
	frame[ETX_IP_CHECKSUM_OFFSET+1] = _LO(checksum ^ 0xffff);
	ipId_++;
	return true;
}



bool EthernetUtilities::createUdpFrame(uint8_t* frame, uint16_t* frameSize,
		uint8_t* dstIp, uint16_t dstPort, uint8_t* srcIp,
		uint16_t srcPort, uint8_t* udpData, uint32_t dataSize,
		uint8_t* srcMac, uint8_t* dstMac)
{
	*frameSize = ETX_UDP_DATA_OFFSET + dataSize;
	if(dataSize > (EMAC_ETH_MAX_FLEN - ETX_UDP_DATA_OFFSET))
		return false;

	memcpy(frame + ETX_ETH_D_MAC_OFFSET, dstMac, ETX_HW_SIZE);
	memcpy(frame + ETX_ETH_S_MAC_OFFSET, srcMac,ETX_HW_SIZE);
	frame[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
	frame[ETX_ETH_ETHER_TYPE_OFFSET+1] = 0x00;//ip

	frame[ETX_IP_IHL_OFFSET] = ETX_IPH_IHL_DEF_VAL;
	frame[ETX_IP_TOS_OFFSET] = ETX_IPH_TOS_DEF_VAL;
	frame[ETX_IP_TOTAL_LEN_OFFSET] =
			_HI(ETX_IP_HEADER_LEN + ETX_UDP_HEADER_LEN + dataSize);
	frame[ETX_IP_TOTAL_LEN_OFFSET+1] =
			_LO(ETX_IP_HEADER_LEN + ETX_UDP_HEADER_LEN + dataSize);
	frame[ETX_IP_ID_OFFSET] = _HI(ipId_);
	frame[ETX_IP_ID_OFFSET+1] = _LO(ipId_);
	frame[ETX_IP_FLAG_FRAGOFFS_OFFSET] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;
	frame[ETX_IP_FLAG_FRAGOFFS_OFFSET+1] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;

	frame[ETX_IP_TTL_OFFSET] = ETX_IPH_TTL_DEF_VAL;
	frame[ETX_IP_PROTO_OFFSET] = ETX_IPH_PROTO_UDP;
	frame[ETX_IP_CHECKSUM_OFFSET] = 0;
	frame[ETX_IP_CHECKSUM_OFFSET+1] = 0;

	memcpy(frame + ETX_IP_SIP_OFFSET, srcIp, ETX_PROTO_SIZE);
	memcpy(frame + ETX_IP_DIP_OFFSET, dstIp, ETX_PROTO_SIZE);

	frame[ETX_UDP_SPORT_OFFSET] = _HI(srcPort);
	frame[ETX_UDP_SPORT_OFFSET+1] = _LO(srcPort);
	frame[ETX_UDP_DPORT_OFFSET] = _HI(dstPort);
	frame[ETX_UDP_DPORT_OFFSET+1] = _LO(dstPort);
	frame[ETX_UDP_LEN_OFFSET] = _HI(dataSize + ETX_UDP_HEADER_LEN);
	frame[ETX_UDP_LEN_OFFSET+1] = _LO(dataSize + ETX_UDP_HEADER_LEN);
	frame[ETX_UDP_CHECKSUM_OFFSET] = 0;
	frame[ETX_UDP_CHECKSUM_OFFSET+1] = 0;

	uint8_t pseudoheader[ETX_UDPPSH_PSEUDOHEADER_LEN];
	memcpy(pseudoheader + ETX_UDPPSH_SIP_OFFSET, srcIp, ETX_PROTO_SIZE);
	memcpy(pseudoheader + ETX_UDPPSH_DIP_OFFSET, dstIp, ETX_PROTO_SIZE);
	pseudoheader[ETX_UDPPSH_NULL_OFFSET] = 0;
	pseudoheader[ETX_UDPPSH_PROTO_OFFSET] = ETX_IPH_PROTO_UDP;
	pseudoheader[ETX_UDPPSH_UDP_LEN_OFFSET] = _HI(dataSize + ETX_UDP_HEADER_LEN);
	pseudoheader[ETX_UDPPSH_UDP_LEN_OFFSET+1] = _LO(dataSize + ETX_UDP_HEADER_LEN);

	memcpy(frame + ETX_UDP_DATA_OFFSET, udpData, dataSize);

	uint16_t checksum = calculateNetworkChecksum(frame + ETX_UDP_HEADER_OFFSET,
			dataSize + ETX_UDP_HEADER_LEN,
			calculateNetworkChecksum(pseudoheader, ETX_UDPPSH_PSEUDOHEADER_LEN, 0));
	frame[ETX_UDP_CHECKSUM_OFFSET] = _HI(checksum ^ 0xffff);
	frame[ETX_UDP_CHECKSUM_OFFSET+1] = _LO(checksum ^ 0xffff);

	checksum = calculateNetworkChecksum(frame + ETX_IP_HEADER_OFFSET,
			ETX_IP_HEADER_LEN, 0);
	frame[ETX_IP_CHECKSUM_OFFSET] = _HI(checksum ^ 0xffff);
	frame[ETX_IP_CHECKSUM_OFFSET+1] = _LO(checksum ^ 0xffff);
	ipId_++;
	return true;
}



uint8_t* EthernetUtilities::getUdpData(uint8_t* frame, uint16_t* dataSize)
{
	uint16_t tempOffset =
			(frame[ETX_IP_IHL_OFFSET] & 0x0f) * 4 + ETX_ETH_HEADER_LEN;
	*dataSize = (frame[tempOffset + 4] << 8) + (frame[tempOffset + 5]) -
			ETX_UDP_HEADER_LEN;
	return frame + tempOffset + ETX_UDP_HEADER_LEN;
}



void EthernetUtilities::changeIp(uint8_t* frame, uint8_t* newDstIp, uint8_t* newSrcIp)
{
	uint16_t offset = 0;
	uint8_t needProtoCsCalculation = 0;
	uint16_t fragmentOffset = ((frame[ETX_IP_FLAG_FRAGOFFS_OFFSET] << 8) |
			(frame[ETX_IP_FLAG_FRAGOFFS_OFFSET + 1])) & 0x1fff;
	if(fragmentOffset == 0) {
		if(frame[ETX_IP_PROTO_OFFSET] == 17) { //UDP
			offset = ETX_ETH_HEADER_LEN +
					(frame[ETX_IP_IHL_OFFSET] & 0x0f) * 4 + 6;
			needProtoCsCalculation = 1;
		}
		else if(frame[ETX_IP_PROTO_OFFSET] == 6) { //TCP
			offset = ETX_ETH_HEADER_LEN +
					(frame[ETX_IP_IHL_OFFSET] & 0x0f) * 4 + 16;
			needProtoCsCalculation = 1;
		}
	}
	if(needProtoCsCalculation) {
		adjustNetworkChecksum(&frame[offset],
				&frame[ETX_IP_SIP_OFFSET], 4, newSrcIp, 4);
		adjustNetworkChecksum(&frame[offset],
				&frame[ETX_IP_DIP_OFFSET], 4, newDstIp, 4);
	}
	memcpy(&frame[ETX_IP_DIP_OFFSET], newDstIp, ETX_PROTO_SIZE);
	memcpy(&frame[ETX_IP_SIP_OFFSET], newSrcIp, ETX_PROTO_SIZE);

	adjustNetworkChecksum(&frame[ETX_IP_CHECKSUM_OFFSET],
			&frame[ETX_IP_SIP_OFFSET], 4, newSrcIp, 4);
	adjustNetworkChecksum(&frame[ETX_IP_CHECKSUM_OFFSET],
			&frame[ETX_IP_DIP_OFFSET], 4, newDstIp, 4);
}



void EthernetUtilities::maskIp(uint8_t* ip, uint8_t* mask)
{
	for(uint8_t i = 0; i < ETX_PROTO_SIZE; i++)
		ip[i] &= mask[i];
}



int EthernetUtilities::compareIp(uint8_t* ip1, uint8_t* ip2, uint8_t* mask){
	uint8_t tmpIp1[ETX_PROTO_SIZE];
	uint8_t tmpIp2[ETX_PROTO_SIZE];
	memcpy(tmpIp1, ip1, ETX_PROTO_SIZE);
	memcpy(tmpIp2, ip2, ETX_PROTO_SIZE);
	maskIp(tmpIp1, mask);
	maskIp(tmpIp2, mask);
	return memcmp(tmpIp1, tmpIp2, ETX_PROTO_SIZE);
}



uint16_t EthernetUtilities::calculateNetworkChecksum(uint8_t* data,uint16_t dataSize,
		uint16_t startValue)
{
	uint32_t sum = 0;
	for(uint16_t i = 0; i < (dataSize-2); i += 2)// sum 16bit without last 16(8)bit
		sum += (((uint16_t)data[i]) << 8) + data[i+1];
	if(dataSize & 1)
		sum += (((uint16_t)data[dataSize-1]) << 8); //if len odd -> add 0x00 and calc sum
	else
		sum+=(((uint16_t)data[dataSize-2]) << 8) + data[dataSize-1];//if len even -> calc sum

	sum += startValue;
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return sum;
}



void EthernetUtilities::adjustNetworkChecksum(uint8_t* chksum, uint8_t* optr,
		uint16_t olen, uint8_t* nptr, uint16_t nlen)
{
  int32_t x = chksum[0] * 256 + chksum[1];
  x = ~x & 0xFFFF;
  for(uint16_t i = 0; i < olen; i = i+2) {
	  int32_t _old = optr[0] * 256 + optr[1];
	  optr += 2;
      x -= _old & 0xffff;
      if (x <= 0){
    	  x--;
    	  x &= 0xffff;
      }
  }
  for(uint16_t i = 0; i < nlen; i = i+2) {
	  int32_t _new = nptr[0] * 256 + nptr[1];
	  nptr += 2;
      x += _new & 0xffff;
      if (x & 0x10000) {
    	  x++;
    	  x &= 0xffff;
      }
  }
  x = ~x & 0xFFFF;
  chksum[0] = x / 256;
  chksum[1] = x & 0xff;
}

}
