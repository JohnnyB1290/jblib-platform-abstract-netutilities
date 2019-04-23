/*
 * Eth_utilities.c
 *
 *  Created on: 12 ���. 2017 �.
 *      Author: Stalker1290
 */
// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Eth_utilities.hpp"

uint16_t Eth_utilities_t::ip_id = 0;


uint16_t Eth_utilities_t::etx_get_ether_type(uint8_t* buf)
{
	 uint16_t retval = ((uint16_t)buf[ETX_ETH_ETHER_TYPE_OFFSET]<<8)|(buf[ETX_ETH_ETHER_TYPE_OFFSET+1]);
	 return retval;
}

void Eth_utilities_t::create_ip_header(uint8_t* outBuff, uint8_t* DIP, uint8_t* SIP, uint32_t DATALEN, uint8_t* SMAC, uint8_t* DMAC, uint8_t protocol)
{
	uint16_t checksum;

	memcpy(outBuff+ETX_ETH_D_MAC_OFFSET,DMAC,ETX_HW_SIZE);
	memcpy(outBuff+ETX_ETH_S_MAC_OFFSET,SMAC,ETX_HW_SIZE);
	outBuff[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
	outBuff[ETX_ETH_ETHER_TYPE_OFFSET+1] = 0x00;//IP

	outBuff[ETX_IP_IHL_OFFSET] = ETX_IPH_IHL_DEF_VAL;
	outBuff[ETX_IP_TOS_OFFSET] = ETX_IPH_TOS_DEF_VAL;
	outBuff[ETX_IP_TOTAL_LEN_OFFSET] = _HI(ETX_IP_HEADER_LEN + DATALEN);
	outBuff[ETX_IP_TOTAL_LEN_OFFSET+1] = _LO(ETX_IP_HEADER_LEN +DATALEN);
	outBuff[ETX_IP_ID_OFFSET] = _HI(Eth_utilities_t::ip_id);
	outBuff[ETX_IP_ID_OFFSET+1] = _LO(Eth_utilities_t::ip_id);
	outBuff[ETX_IP_FLAG_FRAGOFFS_OFFSET] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;
	outBuff[ETX_IP_FLAG_FRAGOFFS_OFFSET+1] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;

	outBuff[ETX_IP_TTL_OFFSET] = ETX_IPH_TTL_DEF_VAL;
	outBuff[ETX_IP_PROTO_OFFSET] = protocol;
	outBuff[ETX_IP_CHECKSUM_OFFSET] = 0;
	outBuff[ETX_IP_CHECKSUM_OFFSET+1] = 0;

	memcpy(outBuff+ETX_IP_SIP_OFFSET,SIP,ETX_PROTO_SIZE);
	memcpy(outBuff+ETX_IP_DIP_OFFSET,DIP,ETX_PROTO_SIZE);

	//calc IP checksum
	checksum = etx_network_checksum(outBuff+ETX_IP_HEADER_OFFSET,ETX_IP_HEADER_LEN,0);
	outBuff[ETX_IP_CHECKSUM_OFFSET] = _HI(checksum^0xffff);
	outBuff[ETX_IP_CHECKSUM_OFFSET+1] = _LO(checksum^0xffff);

	Eth_utilities_t::ip_id++;
}

bool Eth_utilities_t::create_etx_udp_header(uint8_t* outBuff, uint16_t* header_size, uint8_t* DIP, uint16_t DPORT, uint8_t* SIP, uint16_t SPORT, uint32_t DATALEN, uint8_t* SMAC, uint8_t* DMAC)
{
	uint8_t pseudoheader[ETX_UDPPSH_PSEUDOHEADER_LEN];
	uint16_t checksum;//calc udp checksum

	*header_size = ETX_UDP_DATA_OFFSET;

	if(DATALEN > (EMAC_ETH_MAX_FLEN - ETX_UDP_DATA_OFFSET)) return false; // all data do not biger 1536

	memcpy(outBuff+ETX_ETH_D_MAC_OFFSET,DMAC,ETX_HW_SIZE);
	memcpy(outBuff+ETX_ETH_S_MAC_OFFSET,SMAC,ETX_HW_SIZE);
	outBuff[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
	outBuff[ETX_ETH_ETHER_TYPE_OFFSET+1] = 0x00;//IP

	outBuff[ETX_IP_IHL_OFFSET] = ETX_IPH_IHL_DEF_VAL;
	outBuff[ETX_IP_TOS_OFFSET] = ETX_IPH_TOS_DEF_VAL;
	outBuff[ETX_IP_TOTAL_LEN_OFFSET] = _HI(ETX_IP_HEADER_LEN + ETX_UDP_HEADER_LEN + DATALEN);
	outBuff[ETX_IP_TOTAL_LEN_OFFSET+1] = _LO(ETX_IP_HEADER_LEN + ETX_UDP_HEADER_LEN +DATALEN);
	outBuff[ETX_IP_ID_OFFSET] = _HI(Eth_utilities_t::ip_id);
	outBuff[ETX_IP_ID_OFFSET+1] = _LO(Eth_utilities_t::ip_id);
	outBuff[ETX_IP_FLAG_FRAGOFFS_OFFSET] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;
	outBuff[ETX_IP_FLAG_FRAGOFFS_OFFSET+1] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;

	outBuff[ETX_IP_TTL_OFFSET] = ETX_IPH_TTL_DEF_VAL;
	outBuff[ETX_IP_PROTO_OFFSET] = ETX_IPH_PROTO_UDP;
	outBuff[ETX_IP_CHECKSUM_OFFSET] = 0;
	outBuff[ETX_IP_CHECKSUM_OFFSET+1] = 0;

	memcpy(outBuff+ETX_IP_SIP_OFFSET,SIP,ETX_PROTO_SIZE);
	memcpy(outBuff+ETX_IP_DIP_OFFSET,DIP,ETX_PROTO_SIZE);

	outBuff[ETX_UDP_SPORT_OFFSET] = _HI(SPORT);
	outBuff[ETX_UDP_SPORT_OFFSET+1] = _LO(SPORT);
	outBuff[ETX_UDP_DPORT_OFFSET] = _HI(DPORT);
	outBuff[ETX_UDP_DPORT_OFFSET+1] = _LO(DPORT);
	outBuff[ETX_UDP_LEN_OFFSET] = _HI(DATALEN+ETX_UDP_HEADER_LEN);
	outBuff[ETX_UDP_LEN_OFFSET+1] = _LO(DATALEN+ETX_UDP_HEADER_LEN);
	outBuff[ETX_UDP_CHECKSUM_OFFSET] = 0;
	outBuff[ETX_UDP_CHECKSUM_OFFSET+1] = 0;

	memcpy(pseudoheader+ETX_UDPPSH_SIP_OFFSET,SIP,ETX_PROTO_SIZE);
	memcpy(pseudoheader+ETX_UDPPSH_DIP_OFFSET,DIP,ETX_PROTO_SIZE);
	pseudoheader[ETX_UDPPSH_NULL_OFFSET] = 0;
	pseudoheader[ETX_UDPPSH_PROTO_OFFSET] = ETX_IPH_PROTO_UDP;
	pseudoheader[ETX_UDPPSH_UDP_LEN_OFFSET] = _HI(DATALEN+ETX_UDP_HEADER_LEN);
	pseudoheader[ETX_UDPPSH_UDP_LEN_OFFSET+1] = _LO(DATALEN+ETX_UDP_HEADER_LEN);

	checksum = etx_network_checksum(outBuff+ETX_UDP_HEADER_OFFSET,DATALEN+ETX_UDP_HEADER_LEN,etx_network_checksum(pseudoheader,ETX_UDPPSH_PSEUDOHEADER_LEN,0));
	outBuff[ETX_UDP_CHECKSUM_OFFSET] = _HI(checksum^0xffff);
	outBuff[ETX_UDP_CHECKSUM_OFFSET+1] = _LO(checksum^0xffff);

	//calc IP checksum
	checksum = etx_network_checksum(outBuff+ETX_IP_HEADER_OFFSET,ETX_IP_HEADER_LEN,0);
	outBuff[ETX_IP_CHECKSUM_OFFSET] = _HI(checksum^0xffff);
	outBuff[ETX_IP_CHECKSUM_OFFSET+1] = _LO(checksum^0xffff);

	Eth_utilities_t::ip_id++;

	return true;
}

bool Eth_utilities_t::create_etx_udp(uint8_t* outBuff, uint16_t* frame_size, uint8_t* DIP, uint16_t DPORT, uint8_t* SIP, uint16_t SPORT, uint8_t* UDPDATA, uint32_t DATALEN, uint8_t* SMAC, uint8_t* DMAC)
{
	uint8_t pseudoheader[ETX_UDPPSH_PSEUDOHEADER_LEN];
	uint16_t checksum;//calc udp checksum

	*frame_size = ETX_UDP_DATA_OFFSET + DATALEN;

	if(DATALEN > (EMAC_ETH_MAX_FLEN - ETX_UDP_DATA_OFFSET)) return false; // all data do not biger 1536

	memcpy(outBuff+ETX_ETH_D_MAC_OFFSET,DMAC,ETX_HW_SIZE);
	memcpy(outBuff+ETX_ETH_S_MAC_OFFSET,SMAC,ETX_HW_SIZE);
	outBuff[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
	outBuff[ETX_ETH_ETHER_TYPE_OFFSET+1] = 0x00;//IP

	outBuff[ETX_IP_IHL_OFFSET] = ETX_IPH_IHL_DEF_VAL;
	outBuff[ETX_IP_TOS_OFFSET] = ETX_IPH_TOS_DEF_VAL;
	outBuff[ETX_IP_TOTAL_LEN_OFFSET] = _HI(ETX_IP_HEADER_LEN + ETX_UDP_HEADER_LEN + DATALEN);
	outBuff[ETX_IP_TOTAL_LEN_OFFSET+1] = _LO(ETX_IP_HEADER_LEN + ETX_UDP_HEADER_LEN +DATALEN);
	outBuff[ETX_IP_ID_OFFSET] = _HI(Eth_utilities_t::ip_id);
	outBuff[ETX_IP_ID_OFFSET+1] = _LO(Eth_utilities_t::ip_id);
	outBuff[ETX_IP_FLAG_FRAGOFFS_OFFSET] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;
	outBuff[ETX_IP_FLAG_FRAGOFFS_OFFSET+1] = ETX_IPH_FLAG_FRAGOFFS_DEF_VAL;

	outBuff[ETX_IP_TTL_OFFSET] = ETX_IPH_TTL_DEF_VAL;
	outBuff[ETX_IP_PROTO_OFFSET] = ETX_IPH_PROTO_UDP;
	outBuff[ETX_IP_CHECKSUM_OFFSET] = 0;
	outBuff[ETX_IP_CHECKSUM_OFFSET+1] = 0;

	memcpy(outBuff+ETX_IP_SIP_OFFSET,SIP,ETX_PROTO_SIZE);
	memcpy(outBuff+ETX_IP_DIP_OFFSET,DIP,ETX_PROTO_SIZE);

	outBuff[ETX_UDP_SPORT_OFFSET] = _HI(SPORT);
	outBuff[ETX_UDP_SPORT_OFFSET+1] = _LO(SPORT);
	outBuff[ETX_UDP_DPORT_OFFSET] = _HI(DPORT);
	outBuff[ETX_UDP_DPORT_OFFSET+1] = _LO(DPORT);
	outBuff[ETX_UDP_LEN_OFFSET] = _HI(DATALEN+ETX_UDP_HEADER_LEN);
	outBuff[ETX_UDP_LEN_OFFSET+1] = _LO(DATALEN+ETX_UDP_HEADER_LEN);
	outBuff[ETX_UDP_CHECKSUM_OFFSET] = 0;
	outBuff[ETX_UDP_CHECKSUM_OFFSET+1] = 0;

	memcpy(pseudoheader+ETX_UDPPSH_SIP_OFFSET,SIP,ETX_PROTO_SIZE);
	memcpy(pseudoheader+ETX_UDPPSH_DIP_OFFSET,DIP,ETX_PROTO_SIZE);
	pseudoheader[ETX_UDPPSH_NULL_OFFSET] = 0;
	pseudoheader[ETX_UDPPSH_PROTO_OFFSET] = ETX_IPH_PROTO_UDP;
	pseudoheader[ETX_UDPPSH_UDP_LEN_OFFSET] = _HI(DATALEN+ETX_UDP_HEADER_LEN);
	pseudoheader[ETX_UDPPSH_UDP_LEN_OFFSET+1] = _LO(DATALEN+ETX_UDP_HEADER_LEN);

	memcpy(outBuff+ETX_UDP_DATA_OFFSET,UDPDATA,DATALEN);

	checksum = etx_network_checksum(outBuff+ETX_UDP_HEADER_OFFSET,DATALEN+ETX_UDP_HEADER_LEN,etx_network_checksum(pseudoheader,ETX_UDPPSH_PSEUDOHEADER_LEN,0));
	outBuff[ETX_UDP_CHECKSUM_OFFSET] = _HI(checksum^0xffff);
	outBuff[ETX_UDP_CHECKSUM_OFFSET+1] = _LO(checksum^0xffff);

	//calc IP checksum
	checksum = etx_network_checksum(outBuff+ETX_IP_HEADER_OFFSET,ETX_IP_HEADER_LEN,0);
	outBuff[ETX_IP_CHECKSUM_OFFSET] = _HI(checksum^0xffff);
	outBuff[ETX_IP_CHECKSUM_OFFSET+1] = _LO(checksum^0xffff);

	Eth_utilities_t::ip_id++;

	return true;
}

uint8_t* Eth_utilities_t::Get_udp_data_ptr(uint8_t* rec_buff, uint16_t* datalen)
{
	uint16_t Temp_offset;
	Temp_offset = (rec_buff[ETX_IP_IHL_OFFSET]&0x0f)*4 + ETX_ETH_HEADER_LEN;
	*datalen = (rec_buff[Temp_offset + 4]<<8)+(rec_buff[Temp_offset+5]) - ETX_UDP_HEADER_LEN;
	return rec_buff + Temp_offset + ETX_UDP_HEADER_LEN;
}

uint16_t Eth_utilities_t::etx_network_checksum(uint8_t* buf,uint16_t LEN ,uint16_t START_VAL)
{
	uint16_t i;
	uint32_t sum = 0;

	for(i=0;i<(LEN-2);i+=2)// sum 16bit without last 16(8)bit
	{
		sum+=(((uint16_t)buf[i])<<8)+buf[i+1];
	}
	if(LEN&1) sum+=(((uint16_t)buf[LEN-1])<<8);//if len nechet -> add 0x00 and calc sum
	else 		sum+=(((uint16_t)buf[LEN-2])<<8)+buf[LEN-1];//if len chet -> calc sum

	sum+=START_VAL;

	sum = (sum&0xffff) + (sum>>16);
	sum = (sum&0xffff) + (sum>>16);

	return sum;
}

/*
  - chksum ��������� ����������� ����� ������
  - optr ��������� ������ ������ � ������
  - nptr ��������� ����� ������ � ������
  ����������� ���� �������� �������� ������ ��� ������� ������� �������� (�. �., ���� optr ������ ���������� �
   ������� ������ �� ������ ���������) � ������ ����� (�. �., ���� olen � nlen ������ ����� ������ ��������).
*/

void Eth_utilities_t::eth_network_checksumadjust(uint8_t *chksum, uint8_t *optr, uint16_t olen, uint8_t *nptr, uint16_t nlen)
{
  int32_t x, _old, _new;
  uint16_t i;
  x=chksum[0]*256+chksum[1];
  x=~x & 0xFFFF;
  for(i=0; i<olen; i=i+2)
  {
	  _old=optr[0]*256+optr[1]; optr+=2;
      x-=_old & 0xffff;
      if (x<=0)
      {
    	  x--;
    	  x&=0xffff;
      }
  }
  for(i=0; i<nlen; i=i+2)
  {
      _new=nptr[0]*256+nptr[1]; nptr+=2;
      x+=_new & 0xffff;
      if (x & 0x10000)
      {
    	  x++;
    	  x&=0xffff;
      }
  }
  x=~x & 0xFFFF;
  chksum[0]=x/256;
  chksum[1]=x & 0xff;
}

void Eth_utilities_t::Change_Dest_Src_IP(uint8_t* Frame, uint8_t* New_DIP, uint8_t* New_SIP)
{
	uint16_t frag_offset = 0;
	uint16_t offset;
	uint8_t cs_calc_flag = 0;

	frag_offset = (Frame[ETX_IP_FLAG_FRAGOFFS_OFFSET]<<8)|(Frame[ETX_IP_FLAG_FRAGOFFS_OFFSET+1]);
	frag_offset = frag_offset&0x1fff;

	if(frag_offset == 0)
	{
		if(Frame[ETX_IP_PROTO_OFFSET] == 17) //UDP
		{
			offset = ETX_ETH_HEADER_LEN + (Frame[ETX_IP_IHL_OFFSET]&0x0f)*4 + 6;
			cs_calc_flag = 1;
		}
		else if(Frame[ETX_IP_PROTO_OFFSET] == 6) //TCP
		{
			offset = ETX_ETH_HEADER_LEN + (Frame[ETX_IP_IHL_OFFSET]&0x0f)*4 + 16;
			cs_calc_flag = 1;
		}
		else cs_calc_flag = 0;
	}
	else cs_calc_flag = 0;


	if(cs_calc_flag)
	{
		eth_network_checksumadjust(&Frame[offset], &Frame[ETX_IP_SIP_OFFSET], 4, New_SIP, 4);
		eth_network_checksumadjust(&Frame[offset], &Frame[ETX_IP_DIP_OFFSET], 4, New_DIP, 4);
	}

	memcpy(&Frame[ETX_IP_DIP_OFFSET],New_DIP,ETX_PROTO_SIZE);
	memcpy(&Frame[ETX_IP_SIP_OFFSET],New_SIP,ETX_PROTO_SIZE);


	eth_network_checksumadjust(&Frame[ETX_IP_CHECKSUM_OFFSET], &Frame[ETX_IP_SIP_OFFSET], 4, New_SIP, 4);
	eth_network_checksumadjust(&Frame[ETX_IP_CHECKSUM_OFFSET], &Frame[ETX_IP_DIP_OFFSET], 4, New_DIP, 4);
}

void Eth_utilities_t::Mask_ip(uint8_t* IP, uint8_t* mask)
{
	IP[0] &= mask[0];
	IP[1] &= mask[1];
	IP[2] &= mask[2];
	IP[3] &= mask[3];
}

int Eth_utilities_t::compareIp(uint8_t* ip1, uint8_t* ip2, uint8_t* mask){
	uint8_t tmpIp1[4];
	uint8_t tmpIp2[4];

	memcpy(tmpIp1, ip1, 4);
	memcpy(tmpIp2, ip2, 4);

	Eth_utilities_t::Mask_ip(tmpIp1, mask);
	Eth_utilities_t::Mask_ip(tmpIp2, mask);

	return memcmp(tmpIp1, tmpIp2, 4);
}

