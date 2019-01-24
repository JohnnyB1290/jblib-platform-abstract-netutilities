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


#include "dhserver.hpp"

/* DHCP message type */
#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_DECLINE        4
#define DHCP_ACK            5
#define DHCP_NAK            6
#define DHCP_RELEASE        7
#define DHCP_INFORM         8

/* DHCP options */
enum DHCP_OPTIONS
{
	DHCP_PAD                    = 0,
	DHCP_SUBNETMASK             = 1,
	DHCP_ROUTER                 = 3,
	DHCP_DNSSERVER              = 6,
	DHCP_HOSTNAME               = 12,
	DHCP_DNSDOMAIN              = 15,
	DHCP_MTU                    = 26,
	DHCP_BROADCAST              = 28,
	DHCP_PERFORMROUTERDISC      = 31,
	DHCP_STATICROUTE            = 33,
	DHCP_NISDOMAIN              = 40,
	DHCP_NISSERVER              = 41,
	DHCP_NTPSERVER              = 42,
	DHCP_VENDOR                 = 43,
	DHCP_IPADDRESS              = 50,
	DHCP_LEASETIME              = 51,
	DHCP_OPTIONSOVERLOADED      = 52,
	DHCP_MESSAGETYPE            = 53,
	DHCP_SERVERID               = 54,
	DHCP_PARAMETERREQUESTLIST   = 55,
	DHCP_MESSAGE                = 56,
	DHCP_MAXMESSAGESIZE         = 57,
	DHCP_RENEWALTIME            = 58,
	DHCP_REBINDTIME             = 59,
	DHCP_CLASSID                = 60,
	DHCP_CLIENTID               = 61,
	DHCP_USERCLASS              = 77,  /* RFC 3004 */
	DHCP_FQDN                   = 81,
	DHCP_DNSSEARCH              = 119, /* RFC 3397 */
	DHCP_CSR                    = 121, /* RFC 3442 */
	DHCP_MSCSR                  = 249, /* MS code for RFC 3442 */
	DHCP_END                    = 255
};

char DHCP_Server_t::magic_cookie[4] = {0x63,0x82,0x53,0x63};


DHCP_Server_t::DHCP_Server_t(struct netif* LWIP_netif_ptr,uint8_t start_ip, uint8_t ip_count)
{
	this->pcb = (struct udp_pcb*)NULL;
	this->LWIP_netif_ptr = LWIP_netif_ptr;

	this->config.addr[0] = (this->LWIP_netif_ptr->ip_addr.addr & 0xff);
	this->config.addr[1] = (this->LWIP_netif_ptr->ip_addr.addr >> 8) & 0xff;
	this->config.addr[2] = (this->LWIP_netif_ptr->ip_addr.addr >> 16) & 0xff;
	this->config.addr[3] = (this->LWIP_netif_ptr->ip_addr.addr >> 24) & 0xff;
	
	this->config.port = 67;

	memcpy(this->config.dns, this->config.addr, 4);

	this->config.domain = Domain_name;
	this->config.entries = (dhcp_entry_t*) malloc(ip_count*sizeof(dhcp_entry_t));
	if(this->config.entries == (dhcp_entry_t*)NULL) return;

	if(ip_count == 0) return;

	for(uint8_t i = 0; i<ip_count; i++)
	{
		memcpy(this->config.entries[i].addr, this->config.addr, 4);

		if((start_ip + i) == this->config.addr[3])
		{
			start_ip++;
			ip_count--;
		}
		this->config.entries[i].addr[3] = start_ip + i;
		memset(this->config.entries[i].mac,0,6);

		this->config.entries[i].subnet[0] = 255;
		this->config.entries[i].subnet[1] = 255;
		this->config.entries[i].subnet[2] = 255;
		this->config.entries[i].subnet[3] = 0;
		this->config.entries[i].lease = 24*60*60;
	}

	this->config.num_entry = ip_count;

	udp_init();
	this->pcb = udp_new();
	udp_recv(this->pcb, DHCP_Server_t::udp_recv_proc, this);
	udp_bind(this->pcb, &this->LWIP_netif_ptr->ip_addr, this->config.port);
}

DHCP_Server_t::~DHCP_Server_t()
{
	if(this->config.entries != (dhcp_entry_t*)NULL) free(this->config.entries);
	if (this->pcb == NULL) return;
	udp_remove(this->pcb);
	this->pcb = (struct udp_pcb*)NULL;
}


dhcp_entry_t* DHCP_Server_t::entry_by_ip(uint8_t* ip)
{
	for (int i = 0; i < this->config.num_entry; i++)
	{
		if(memcmp(this->config.entries[i].addr, ip, 4) == 0) return &this->config.entries[i];
	}
	return NULL;
}

dhcp_entry_t* DHCP_Server_t::entry_by_mac(uint8_t* mac)
{
	for (int i = 0; i < this->config.num_entry; i++)
	{
		if (memcmp(this->config.entries[i].mac, mac, 6) == 0) return &this->config.entries[i];
	}
	return NULL;
}

bool DHCP_Server_t::is_vacant(dhcp_entry_t* entry)
{
	return memcmp("\0\0\0\0\0", entry->mac, 6) == 0;
}

void DHCP_Server_t::free_entry(dhcp_entry_t* entry)
{
	memset(entry->mac, 0, 6);
}

dhcp_entry_t* DHCP_Server_t::vacant_address(void)
{
	for (int i = 0; i < this->config.num_entry; i++)
	{
		if (this->is_vacant(this->config.entries + i))	return this->config.entries + i;
	}
	return NULL;
}


uint8_t* DHCP_Server_t::find_dhcp_option(uint8_t* attrs, int size, uint8_t attr)
{
	int i = 0;
	while ((i + 1) < size)
	{
		int next = i + attrs[i + 1] + 2;
		if (next > size) return NULL;
		if (attrs[i] == attr)
			return attrs + i;
		i = next;
	}
	return NULL;
}

void DHCP_Server_t::fill_options(uint8_t msg_type, dhcp_entry_t* entry)
{
	static uint8_t* dp_options_ptr;

	dp_options_ptr = (uint8_t *)this->dhcp_data.dp_options;

	*dp_options_ptr++ = DHCP_MESSAGETYPE;
	*dp_options_ptr++ = 1;
	*dp_options_ptr++ = msg_type;

	/* dhcp server identifier */
	*dp_options_ptr++ = DHCP_SERVERID;
	*dp_options_ptr++ = 4;
	memcpy(dp_options_ptr, this->config.addr, 4);
	dp_options_ptr += 4;

	/* lease time */
	*dp_options_ptr++ = DHCP_LEASETIME;
	*dp_options_ptr++ = 4;
	*dp_options_ptr++ = (entry->lease >> 24) & 0xFF;
	*dp_options_ptr++ = (entry->lease >> 16) & 0xFF;
	*dp_options_ptr++ = (entry->lease >> 8) & 0xFF;
	*dp_options_ptr++ = (entry->lease >> 0) & 0xFF;

	/* subnet mask */
	*dp_options_ptr++ = DHCP_SUBNETMASK;
	*dp_options_ptr++ = 4;
	memcpy(dp_options_ptr, entry->subnet,4);
	dp_options_ptr += 4;

	/* router */
	if (memcmp("\0\0\0", this->config.addr, 4) != 0)
	{
		*dp_options_ptr++ = DHCP_ROUTER;
		*dp_options_ptr++ = 4;
		memcpy((void*) dp_options_ptr, (void*) this->config.addr, 4);
		dp_options_ptr += 4;
	}

	/* domain name */
	if (this->config.domain != NULL)
	{
		int len = strlen(this->config.domain);
		*dp_options_ptr++ = DHCP_DNSDOMAIN;
		*dp_options_ptr++ = len;
		memcpy((void*)dp_options_ptr, this->config.domain, len);
		dp_options_ptr += len;
	}

	/* domain name server (DNS) */
	if ( memcmp("\0\0\0", this->config.dns, 4) != 0)
	{
		*dp_options_ptr++ = DHCP_DNSSERVER;
		*dp_options_ptr++ = 4;
		memcpy((void*) dp_options_ptr, (void*) this->config.dns, 4);
		dp_options_ptr += 4;
	}
	/* end */
	*dp_options_ptr++ = DHCP_END;
}

void DHCP_Server_t::udp_recv_proc(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	static uint8_t* ptr;
	static dhcp_entry_t* entry;
	DHCP_Server_t* dhcpServer_ptr = (DHCP_Server_t*)arg;	
	struct pbuf* pp = NULL;
	
	ptr = NULL;
	entry = NULL;
	
	int n = p->len;
	if (n > sizeof(dhcpServer_ptr->dhcp_data)) n = sizeof(dhcpServer_ptr->dhcp_data);
	memcpy(&dhcpServer_ptr->dhcp_data, p->payload, n);
	switch (dhcpServer_ptr->dhcp_data.dp_options[2])
	{
		case DHCP_DISCOVER:
			entry = dhcpServer_ptr->entry_by_mac(dhcpServer_ptr->dhcp_data.dp_chaddr);
			if (entry == NULL) entry = dhcpServer_ptr->vacant_address();
			if (entry == NULL) break;

			dhcpServer_ptr->dhcp_data.dp_op = 2; /* reply */
			dhcpServer_ptr->dhcp_data.dp_secs = 0;
			dhcpServer_ptr->dhcp_data.dp_flags = 0;
			memcpy(dhcpServer_ptr->dhcp_data.dp_yiaddr, entry->addr, 4);
			memcpy(dhcpServer_ptr->dhcp_data.dp_magic, DHCP_Server_t::magic_cookie, 4);
			memset(dhcpServer_ptr->dhcp_data.dp_options, 0, sizeof(dhcpServer_ptr->dhcp_data.dp_options));

			dhcpServer_ptr->fill_options(DHCP_OFFER, entry);

			pp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcpServer_ptr->dhcp_data), PBUF_RAM);
			if (pp == NULL) break;
			memcpy(pp->payload, &dhcpServer_ptr->dhcp_data, sizeof(dhcpServer_ptr->dhcp_data));
			udp_sendto_if(upcb, pp, IP_ADDR_BROADCAST, port, dhcpServer_ptr->LWIP_netif_ptr);
			pbuf_free(pp);
			break;

		case DHCP_REQUEST:
			/* 1. find requested ipaddr in option list */
			ptr = dhcpServer_ptr->find_dhcp_option(dhcpServer_ptr->dhcp_data.dp_options, sizeof(dhcpServer_ptr->dhcp_data.dp_options), DHCP_IPADDRESS);
			if (ptr == NULL) break;
			if (ptr[1] != 4) break;
			ptr += 2;

			/* 2. does hw-address registered? */
			entry = dhcpServer_ptr->entry_by_mac(dhcpServer_ptr->dhcp_data.dp_chaddr);
			if (entry != NULL) dhcpServer_ptr->free_entry(entry);

			/* 3. find requested ipaddr */
			entry = dhcpServer_ptr->entry_by_ip(ptr);
			if (entry == NULL) break;
			if (!dhcpServer_ptr->is_vacant(entry)) break;

			/* 4. fill struct fields */
			memcpy(dhcpServer_ptr->dhcp_data.dp_yiaddr, ptr, 4);
			dhcpServer_ptr->dhcp_data.dp_op = 2; /* reply */
			dhcpServer_ptr->dhcp_data.dp_secs = 0;
			dhcpServer_ptr->dhcp_data.dp_flags = 0;
			memcpy(dhcpServer_ptr->dhcp_data.dp_magic, DHCP_Server_t::magic_cookie, 4);

			/* 5. fill options */
			memset(dhcpServer_ptr->dhcp_data.dp_options, 0, sizeof(dhcpServer_ptr->dhcp_data.dp_options));

			dhcpServer_ptr->fill_options(DHCP_ACK, entry);

			/* 6. send ACK */
			pp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcpServer_ptr->dhcp_data), PBUF_RAM);
			if (pp == NULL) break;
			memcpy(entry->mac, dhcpServer_ptr->dhcp_data.dp_chaddr, 6);
			memcpy(pp->payload, &dhcpServer_ptr->dhcp_data, sizeof(dhcpServer_ptr->dhcp_data));
			udp_sendto_if(upcb, pp, IP_ADDR_BROADCAST, port, dhcpServer_ptr->LWIP_netif_ptr);
			pbuf_free(pp);
			break;

			case DHCP_INFORM:
				
			/* 1. find requested ipaddr in option list */
			ptr = dhcpServer_ptr->dhcp_data.dp_ciaddr;

			/* 2. does hw-address registered? */
			entry = dhcpServer_ptr->entry_by_mac(dhcpServer_ptr->dhcp_data.dp_chaddr);
			if (entry != NULL) dhcpServer_ptr->free_entry(entry);

			/* 3. find requested ipaddr */
			entry = dhcpServer_ptr->entry_by_ip(ptr);
			if (entry == NULL) break;
			if (!dhcpServer_ptr->is_vacant(entry)) break;

			/* 4. fill struct fields */
			memcpy(dhcpServer_ptr->dhcp_data.dp_yiaddr, ptr, 4);
			dhcpServer_ptr->dhcp_data.dp_op = 2; /* reply */
			dhcpServer_ptr->dhcp_data.dp_secs = 0;
			dhcpServer_ptr->dhcp_data.dp_flags = 0;
			memcpy(dhcpServer_ptr->dhcp_data.dp_magic, DHCP_Server_t::magic_cookie, 4);

			/* 5. fill options */
			memset(dhcpServer_ptr->dhcp_data.dp_options, 0, sizeof(dhcpServer_ptr->dhcp_data.dp_options));

			dhcpServer_ptr->fill_options(DHCP_ACK, entry);

			memset(dhcpServer_ptr->dhcp_data.dp_ciaddr,0,4);
			/* 6. send ACK */
			pp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcpServer_ptr->dhcp_data), PBUF_RAM);
			if (pp == NULL) break;
			memcpy(entry->mac, dhcpServer_ptr->dhcp_data.dp_chaddr, 6);
			memcpy(pp->payload, &dhcpServer_ptr->dhcp_data, sizeof(dhcpServer_ptr->dhcp_data));
			udp_sendto_if(upcb, pp, IP_ADDR_BROADCAST, port, dhcpServer_ptr->LWIP_netif_ptr);
			pbuf_free(pp);
			break;
	
		default:
				break;
	}
	pbuf_free(p);
}

