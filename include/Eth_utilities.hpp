/*
 * Eth_utilities.hpp
 *
 *  Created on: 12 окт. 2017 г.
 *      Author: Stalker1290
 */

#ifndef ETH_UTILITIES_HPP_
#define ETH_UTILITIES_HPP_


#include "stdbool.h"
#include "stdint.h"
#include "string.h"
#include "Defines.h"
#include "chip.h"

#define _HI(x)	(((x)>>8)&0xff)
#define _LO(x)	((x)&0xff)

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

class Eth_utilities_t
{
public:
	static void create_ip_header(uint8_t* outBuff, uint8_t* DIP, uint8_t* SIP, uint32_t DATALEN, uint8_t* SMAC, uint8_t* DMAC, uint8_t protocol);
	static uint16_t etx_get_ether_type(uint8_t* buf);
	static bool create_etx_udp_header(uint8_t* outBuff, uint16_t* header_size, uint8_t* DIP, uint16_t DPORT, uint8_t* SIP, uint16_t SPORT, uint32_t DATALEN, uint8_t* SMAC, uint8_t* DMAC);
	static bool create_etx_udp(uint8_t* outBuff, uint16_t* frame_size, uint8_t* DIP, uint16_t DPORT, uint8_t* SIP, uint16_t SPORT, uint8_t* UDPDATA, uint32_t DATALEN, uint8_t* SMAC, uint8_t* DMAC);
	static uint16_t etx_network_checksum(uint8_t* buf,uint16_t LEN ,uint16_t START_VAL);
	static uint8_t* Get_udp_data_ptr(uint8_t* rec_buff, uint16_t* datalen);
	static void eth_network_checksumadjust(uint8_t *chksum, uint8_t *optr, uint16_t olen, uint8_t *nptr, uint16_t nlen);
	static void Change_Dest_Src_IP(uint8_t* Frame, uint8_t* New_DIP, uint8_t* New_SIP);
	static void Mask_ip(uint8_t* IP, uint8_t* mask);
	static int compareIp(uint8_t* ip1, uint8_t* ip2, uint8_t* mask);

	static uint16_t ip_id;
};



#endif /* ETH_UTILITIES_HPP_ */
