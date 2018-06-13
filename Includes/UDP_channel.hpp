/*
 * UDP_channel.hpp
 *
 *  Created on: 27.10.2017
 *      Author: Stalker1290
 */

#ifndef UDP_CHANNEL_HPP_
#define UDP_CHANNEL_HPP_

#include "Void_Channel.hpp"
#include "Ethernet_router.hpp"

class UDP_Channel_t:public void_channel_t
{
public:
	UDP_Channel_t(uint8_t* SRC_IP, uint16_t SRC_PORT, uint8_t* DST_IP, uint16_t DST_PORT, Ethernet_t* Eth_interface_ptr);
	UDP_Channel_t(uint16_t SRC_PORT, uint8_t* DST_IP, uint16_t DST_PORT, Ethernet_t* Eth_interface_ptr);
	UDP_Channel_t(uint16_t SRC_PORT, uint8_t* DST_IP, uint16_t DST_PORT);
	virtual ~UDP_Channel_t(void);
	virtual void Initialize(void* (*mem_alloc)(size_t),uint16_t tx_buf_size, Channel_Call_Interface_t* call_interface_ptr);
	virtual void DEInitialize(void);
	virtual void Tx(uint8_t *mes,uint16_t m_size,void* param);
	virtual void GetParameter(uint8_t ParamName, void* ParamValue);
	virtual void SetParameter(uint8_t ParamName, void* ParamValue);
private:
	void Constructor(uint8_t* SRC_IP, uint16_t SRC_PORT, uint8_t* DST_IP, uint16_t DST_PORT, Ethernet_t* Eth_interface_ptr);
	static void udp_recv_fn(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port);
	struct udp_pcb* UDP_pcb_ptr;
	ip_addr_t src_ipaddr;
	uint16_t SRC_PORT;
	ip_addr_t dst_ipaddr;
	uint16_t DST_PORT;
	netif* 	Netif_ptr;
	Channel_Call_Interface_t* call_interface_ptr;
	struct pbuf* p_tx;
};




#endif /* UDP_CHANNEL_HPP_ */
