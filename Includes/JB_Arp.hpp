/*
 * JB_Arp.hpp
 *
 *  Created on: 18.10.2017 ã.
 *      Author: Stalker1290
 */
#ifndef JB_ARP_HPP_
#define JB_ARP_HPP_

#include "stdbool.h"
#include "stdint.h"
#include "string.h"
#include "chip.h"
#include "Defines.h"
#include "Eth_utilities.hpp"
#include "Void_Ethernet.hpp"
#include "Common_interfaces.hpp"

#pragma pack(push, 1)
typedef struct {
	uint8_t ip[ETX_IP_TABLE_FOR_ARP_MAX_NUMBER][ETX_PROTO_SIZE];
	uint16_t total_ip_num;
}IP_table_for_arp_t;

typedef struct {
	uint8_t ip[ETX_ARP_TABLE_MAX_NUMBER][ETX_PROTO_SIZE]; // IP
	uint8_t mac[ETX_ARP_TABLE_MAX_NUMBER][ETX_HW_SIZE]; //MAC
	uint32_t time_records[ETX_ARP_TABLE_MAX_NUMBER];//time of last arp update
	uint16_t total_ip_num;// number of records arp table
}arp_table_t;
#pragma pack(pop)

class Arp_updater_t:public Ethernet_listener_t, Callback_Interface_t
{
public:
	Arp_updater_t(Ethernet_t* Eth_adapter_ptr);
	virtual ~Arp_updater_t(void);
	virtual void Parse_frame(EthernetFrame* frame_ptr,uint16_t frame_size, Ethernet_t* Eth_adapter_ptr, void* Parameter);
	void Send_arp_request(uint8_t* IP_d);
	void Add_ip_in_ip_table_for_arp(uint8_t* IP);
	bool Is_ip_in_ip_table_for_arp(uint8_t* IP);
	bool Get_MAC_from_IP(uint8_t* IP, uint8_t* MAC);
	bool Get_IP_from_MAC(uint8_t* MAC, uint8_t* IP);
	virtual void void_callback(void* Intf_ptr, void* parameters);
	arp_table_t* Get_arp_table_ptr(void);
private:
	uint8_t* MAC;
	Ethernet_t* Eth_adapter_ptr;
	IP_table_for_arp_t ip_table;
	arp_table_t arp_table;


	int16_t get_index_arp_table_from_ip(uint8_t* IP);
	void create_etx_arp_reply(uint8_t* requBuff, uint8_t* outBuff, uint8_t* S_MAC, uint16_t* frame_size);
	void create_etx_arp_request(const uint8_t* IP, uint8_t* S_MAC,uint8_t* S_IP, uint8_t* outBuff, uint16_t* frame_size);
};

#endif /* JB_ARP_HPP_ */
