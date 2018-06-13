/*
 * UDP_channel.cpp
 *
 *  Created on: 27.10.2017
 *      Author: Stalker1290
 */

#include "UDP_channel.hpp"
#include "lwip/udp.h"
#include "lwip/igmp.h"


UDP_Channel_t::UDP_Channel_t(uint16_t SRC_PORT, uint8_t* DST_IP, uint16_t DST_PORT, Ethernet_t* Eth_interface_ptr):void_channel_t()
{
	this->Constructor(NULL, SRC_PORT, DST_IP, DST_PORT, Eth_interface_ptr);
}

UDP_Channel_t::UDP_Channel_t(uint16_t SRC_PORT, uint8_t* DST_IP, uint16_t DST_PORT):void_channel_t()
{
	this->Constructor(NULL, SRC_PORT, DST_IP, DST_PORT, NULL);
}

UDP_Channel_t::UDP_Channel_t(uint8_t* SRC_IP, uint16_t SRC_PORT, uint8_t* DST_IP, uint16_t DST_PORT, Ethernet_t* Eth_interface_ptr):void_channel_t()
{
	this->Constructor(SRC_IP, SRC_PORT, DST_IP, DST_PORT, Eth_interface_ptr);
}

void UDP_Channel_t::Constructor(uint8_t* SRC_IP, uint16_t SRC_PORT, uint8_t* DST_IP, uint16_t DST_PORT, Ethernet_t* Eth_interface_ptr)
{
	this->p_tx = NULL;
	if(SRC_IP != NULL) IP4_ADDR(&this->src_ipaddr, SRC_IP[0], SRC_IP[1], SRC_IP[2], SRC_IP[3]);
	else this->src_ipaddr = ip_addr_any;

	this->SRC_PORT = SRC_PORT;

	if(DST_IP != NULL) IP4_ADDR(&this->dst_ipaddr, DST_IP[0], DST_IP[1], DST_IP[2], DST_IP[3]);
	else this->dst_ipaddr = ip_addr_any;

	this->DST_PORT = DST_PORT;

	if(Eth_interface_ptr != (Ethernet_t*)NULL) this->Netif_ptr = Ethernet_router_t::get_Ethernet_router()->get_LWIP_netif(Eth_interface_ptr);
	else this->Netif_ptr = (netif*)NULL;

#if LWIP_IGMP == 1
	if(ip4_addr_ismulticast(&this->dst_ipaddr)) igmp_joingroup(&this->src_ipaddr,&this->dst_ipaddr);
#endif

	udp_init();
	this->UDP_pcb_ptr = udp_new();
	udp_recv(this->UDP_pcb_ptr, UDP_Channel_t::udp_recv_fn, this);
	udp_bind(this->UDP_pcb_ptr, &this->src_ipaddr, this->SRC_PORT);
}

UDP_Channel_t::~UDP_Channel_t(void)
{
	udp_remove(this->UDP_pcb_ptr);
}

void UDP_Channel_t::Tx(uint8_t *mes,uint16_t m_size,void* param)
{
	uint32_t unbuf_size = m_size;
	uint32_t payload_size = 0;
	uint8_t* mes_ptr = mes;

	while(unbuf_size>0)
	{
		payload_size = MIN(unbuf_size,TCP_MSS);
		this->p_tx = pbuf_alloc(PBUF_TRANSPORT, payload_size, PBUF_RAM);
		if (this->p_tx == NULL) break;

		memcpy(this->p_tx->payload,mes_ptr, payload_size);
		mes_ptr = mes_ptr + payload_size*sizeof(uint8_t);

		if(this->Netif_ptr == (netif*)NULL) udp_sendto(this->UDP_pcb_ptr,this->p_tx,&this->dst_ipaddr,this->DST_PORT);
		else udp_sendto_if(this->UDP_pcb_ptr,this->p_tx,&this->dst_ipaddr,this->DST_PORT,this->Netif_ptr);

		pbuf_free(this->p_tx);

		unbuf_size = unbuf_size - payload_size;
	}
}

void UDP_Channel_t::udp_recv_fn(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
	struct pbuf* p_next = p;
	UDP_Channel_t* UDP_ch_instance_ptr = (UDP_Channel_t*)arg;
	if(UDP_ch_instance_ptr->call_interface_ptr != (Channel_Call_Interface_t*)NULL)
	{
		while(p_next != NULL)
		{
			UDP_ch_instance_ptr->call_interface_ptr->channel_callback((uint8_t*)p->payload, p->len, UDP_ch_instance_ptr, NULL);
			p_next = p_next->next;
		}
	}
	pbuf_free(p);
}

void UDP_Channel_t::Initialize(void* (*mem_alloc)(size_t),uint16_t tx_buf_size, Channel_Call_Interface_t* call_interface_ptr)
{
	this->call_interface_ptr = call_interface_ptr;
}

void UDP_Channel_t::DEInitialize(void)
{
	this->call_interface_ptr = (Channel_Call_Interface_t*)NULL;
}

void UDP_Channel_t::GetParameter(uint8_t ParamName, void* ParamValue)
{

}

void UDP_Channel_t::SetParameter(uint8_t ParamName, void* ParamValue)
{

}
