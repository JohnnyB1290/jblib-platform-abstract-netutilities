/*
 * Void_Ethernet.hpp
 *
 *  Created on: 12.10.2017
 *      Author: Stalker1290
 */

#include "chip.h"

#ifdef USE_LWIP
//#include "lwip/pbuf.h"
#endif

#ifndef VOID_ETHERNET_HPP_
#define VOID_ETHERNET_HPP_


class Ethernet_t
{
public:
	Ethernet_t(void){}
	virtual ~Ethernet_t(void){}
	virtual void Initialize(void) = 0;
	virtual void Start(void) = 0;
	virtual void ResetDevice(void) = 0;
	virtual void GetParameter(uint8_t ParamName, void* ParamValue) = 0;
	virtual void SetParameter(uint8_t ParamName, void* ParamValue) = 0;
	virtual uint8_t Check_if_TX_queue_not_full(void) = 0;
	virtual void Add_to_TX_queue(EthernetFrame* mes,uint16_t m_size) = 0;
#ifdef USE_LWIP
	virtual void Add_to_TX_queue(struct pbuf* p) = 0;
#endif
	virtual uint16_t Pull_out_RX_Frame(EthernetFrame* Frame) = 0;

	uint32_t txFramesCnt = 0;
	uint32_t txBytesCnt = 0;
	uint32_t rxFramesCnt = 0;
	uint32_t rxBytesCnt = 0;
	uint32_t errorsCnt = 0;
};

typedef enum
{
	MAC_param = 0,
	Tx_Unlock_param,
	LINK_param,
	speed_param,
	name_param,
}Ethernet_param_t;

typedef enum
{
	speed_10Mbit = 0,
	speed_100Mbit = 1,
	speed_autoneg = 2,
	speed_1000Mbit = 3,
}Ethernet_speed_t;


class Ethernet_listener_t
{
public:
	Ethernet_listener_t(void){}
	virtual ~Ethernet_listener_t(void){}
	virtual void Parse_frame(EthernetFrame* frame_ptr,uint16_t frame_size, Ethernet_t* Eth_adapter_ptr, void* Parameter) = 0;
};

#endif /* VOID_ETHERNET_HPP_ */
