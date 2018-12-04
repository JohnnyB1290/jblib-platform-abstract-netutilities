/*
 * Ethernet_router.hpp
 *
 *  Created on: 19.10.2017
 *      Author: Stalker1290
 */

#ifndef ETHERNET_ROUTER_HPP_
#define ETHERNET_ROUTER_HPP_

#include "chip.h"
#include "Void_Ethernet.hpp"
#include "Common_interfaces.hpp"
#include "Defines.h"
#include "JB_Arp.hpp"
#include "arch/systick_arch.h"
#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "arch/ethernetif.h"

typedef enum{
	LWIP_MS_CALL = 0,
	LWIP_LINK_CHECK_CALL = 1,
	ROUTE_FRAMES_CALL = 2
}EthRouterCallType;

class Ethernet_router_t:public Callback_Interface_t
{
public:
	static Ethernet_router_t* get_Ethernet_router(void);
	static Arp_updater_t* Get_ARP_Updater(Ethernet_t* Eth_interface_ptr);
	void Start(uint8_t NRT_Timer_num);
	struct netif* get_LWIP_netif(Ethernet_t* Eth_interface_ptr);
	void Add_Ethernet_interface(Ethernet_t* Eth_interface_ptr, uint8_t* IP, uint8_t* GW, uint8_t* NETMASK);
	void Add_Ethernet_listener(Ethernet_listener_t* listener, Ethernet_t* Eth_interface_ptr);
	void Add_Ethernet_listener(Ethernet_listener_t* listener);
	void Delete_Ethernet_listener(Ethernet_listener_t* listener, Ethernet_t* Eth_interface_ptr);
	void Delete_Ethernet_listener(Ethernet_listener_t* listener);
	virtual void void_callback(void* Intf_ptr, void* parameters);
	void setParsePeriodUs(uint32_t parsePeriodUs);
private:
	Ethernet_router_t(void);
	void routeFrames(void);

	uint32_t parsePeriodUs;

	static C_void_callback_t* LWIP_ms_call_interface_ptr;
	static Ethernet_router_t* Router_ptr;
	static Ethernet_t* Interface_ptrs[ETH_ROUTER_NUM_OF_NETWORK_INTF];
	static struct netif LWIP_netif[ETH_ROUTER_NUM_OF_NETWORK_INTF];
	static Arp_updater_t* Arp_updater_ptrs[ETH_ROUTER_NUM_OF_NETWORK_INTF];
	static uint8_t interfaces_count;
	static EthernetFrame In_Frame;
	static uint16_t In_Frame_size;
	static Ethernet_listener_t* listeners[ETH_ROUTER_NUM_OF_NETWORK_INTF][ETH_ROUTER_NUM_OF_LISTENERS];
	static uint8_t NRT_Timer_num;
};


#endif /* ETHERNET_ROUTER_HPP_ */
