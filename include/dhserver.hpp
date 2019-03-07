/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 by Sergey Fetisov <fsenok@gmail.com>, Stalker1290
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * version: 1.0 demo (7.02.2015)
 * brief:   tiny dhcp ipv4 server using lwip (pcb)
 * ref:     https://lists.gnu.org/archive/html/lwip-users/2012-12/msg00016.html
 */

#ifndef DHSERVER_HPP
#define DHSERVER_HPP

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "lwip/udp.h"


typedef struct dhcp_entry
{
	uint8_t  mac[6];
	uint8_t  addr[4];
	uint8_t  subnet[4];
	uint32_t lease;
} dhcp_entry_t;

typedef struct dhcp_config
{
	uint8_t			addr[4];
	uint16_t      	port;
	uint8_t			dns[4];
	char*			domain;
	int				num_entry;
	dhcp_entry_t*	entries;
} dhcp_config_t;

typedef struct
{
    uint8_t  dp_op;           /* packet opcode type */
    uint8_t  dp_htype;        /* hardware addr type */
    uint8_t  dp_hlen;         /* hardware addr length */
    uint8_t  dp_hops;         /* gateway hops */
    uint32_t dp_xid;          /* transaction ID */
    uint16_t dp_secs;         /* seconds since boot began */
    uint16_t dp_flags;
    uint8_t  dp_ciaddr[4];    /* client IP address */
    uint8_t  dp_yiaddr[4];    /* 'your' IP address */
    uint8_t  dp_siaddr[4];    /* server IP address */
    uint8_t  dp_giaddr[4];    /* gateway IP address */
    uint8_t  dp_chaddr[16];   /* client hardware address */
    uint8_t  dp_legacy[192];
    uint8_t  dp_magic[4];
    uint8_t  dp_options[275]; /* options area */
} DHCP_TYPE;


class DHCP_Server_t
{
public:
	DHCP_Server_t(struct netif* LWIP_netif_ptr,uint8_t start_ip, uint8_t ip_count);
	~DHCP_Server_t();
private:
	dhcp_entry_t* entry_by_ip(uint8_t* ip);
	dhcp_entry_t* entry_by_mac(uint8_t* mac);
	bool is_vacant(dhcp_entry_t* entry);
	dhcp_entry_t* vacant_address(void);
	void free_entry(dhcp_entry_t* entry);
	uint8_t* find_dhcp_option(uint8_t* attrs, int size, uint8_t attr);
	void fill_options(uint8_t msg_type, dhcp_entry_t* entry);
	static void udp_recv_proc(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

	DHCP_TYPE dhcp_data;
	struct udp_pcb* pcb;
	dhcp_config_t config;
	static char magic_cookie[4];
	struct netif* LWIP_netif_ptr;

};
#endif /* DHSERVER_HPP */
