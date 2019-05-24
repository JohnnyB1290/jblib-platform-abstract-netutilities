/*
 * The MIT License (MIT)
 *
 * Copyright © 2015 by Sergey Fetisov <fsenok@gmail.com>
 * Copyright © 2019 Evgeniy Ivanov. Contacts: <strelok1290@gmail.com>
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
// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <string.h>
#include "jb_common.h"
#include "DhcpServer.hpp"

/* DHCP message type */
#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_DECLINE        4
#define DHCP_ACK            5
#define DHCP_NAK            6
#define DHCP_RELEASE        7
#define DHCP_INFORM         8

namespace jblib::ethutilities
{

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

char DhcpServer::magicCookie_[4] = {0x63,0x82,0x53,0x63};



DhcpServer::DhcpServer(struct netif* netifStruct,uint8_t startIp, uint8_t ipCount)
{
	memset(&this->dhcp_data_, 0, sizeof(DHCP_TYPE));
	memset(&this->config_, 0, sizeof(dhcp_config_t));
	this->netifStruct_ = netifStruct;
	this->config_.addr[0] = (this->netifStruct_->ip_addr.addr & 0xff);
	this->config_.addr[1] = (this->netifStruct_->ip_addr.addr >> 8) & 0xff;
	this->config_.addr[2] = (this->netifStruct_->ip_addr.addr >> 16) & 0xff;
	this->config_.addr[3] = (this->netifStruct_->ip_addr.addr >> 24) & 0xff;
	this->config_.port = 67;
	memcpy(this->config_.dns, this->config_.addr, 4);
	this->config_.domain = DHCP_SERVER_DOMAIN_NAME;
	this->config_.entries = (dhcp_entry_t*) malloc_s(ipCount*sizeof(dhcp_entry_t));
	if(this->config_.entries == (dhcp_entry_t*)NULL)
		return;
	if(ipCount == 0)
		return;
	for(uint32_t i = 0; i < ipCount; i++) {
		memcpy(this->config_.entries[i].addr, this->config_.addr, 4);
		if((startIp + i) == this->config_.addr[3]) {
			startIp++;
			ipCount--;
		}
		this->config_.entries[i].addr[3] = startIp + i;
		memset(this->config_.entries[i].mac, 0, 6);

		this->config_.entries[i].subnet[0] = 255;
		this->config_.entries[i].subnet[1] = 255;
		this->config_.entries[i].subnet[2] = 255;
		this->config_.entries[i].subnet[3] = 0;
		this->config_.entries[i].lease = 24*60*60;
	}

	this->config_.num_entry = ipCount;

	udp_init();
	this->pcb_ = udp_new();
	udp_recv(this->pcb_, udp_recv_proc, this);
	udp_bind(this->pcb_, &this->netifStruct_->ip_addr, this->config_.port);
}



DhcpServer::~DhcpServer()
{
	if(this->config_.entries != (dhcp_entry_t*)NULL)
		free_s(this->config_.entries);
	if (this->pcb_ == NULL)
		return;
	udp_remove(this->pcb_);
	this->pcb_ = (struct udp_pcb*)NULL;
}



dhcp_entry_t* DhcpServer::entry_by_ip(uint8_t* ip)
{
	for (int i = 0; i < this->config_.num_entry; i++) {
		if(memcmp(this->config_.entries[i].addr, ip, 4) == 0)
			return &this->config_.entries[i];
	}
	return NULL;
}



dhcp_entry_t* DhcpServer::entry_by_mac(uint8_t* mac)
{
	for (int i = 0; i < this->config_.num_entry; i++) {
		if (memcmp(this->config_.entries[i].mac, mac, 6) == 0)
			return &this->config_.entries[i];
	}
	return NULL;
}



bool DhcpServer::is_vacant(dhcp_entry_t* entry)
{
	return (memcmp("\0\0\0\0\0", entry->mac, 6) == 0);
}



void DhcpServer::free_entry(dhcp_entry_t* entry)
{
	memset(entry->mac, 0, 6);
}

dhcp_entry_t* DhcpServer::vacant_address(void)
{
	for (int i = 0; i < this->config_.num_entry; i++) {
		if (this->is_vacant(this->config_.entries + i))
			return this->config_.entries + i;
	}
	return NULL;
}



uint8_t* DhcpServer::find_dhcp_option(uint8_t* attrs, int size, uint8_t attr)
{
	int i = 0;
	while ((i + 1) < size) {
		int next = i + attrs[i + 1] + 2;
		if (next > size) return NULL;
		if (attrs[i] == attr)
			return attrs + i;
		i = next;
	}
	return NULL;
}



void DhcpServer::fill_options(uint8_t msg_type, dhcp_entry_t* entry)
{
	static uint8_t* dp_options_ptr = NULL;

	dp_options_ptr = (uint8_t *)this->dhcp_data_.dp_options;

	*dp_options_ptr++ = DHCP_MESSAGETYPE;
	*dp_options_ptr++ = 1;
	*dp_options_ptr++ = msg_type;

	/* dhcp server identifier */
	*dp_options_ptr++ = DHCP_SERVERID;
	*dp_options_ptr++ = 4;
	memcpy(dp_options_ptr, this->config_.addr, 4);
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
	if (memcmp("\0\0\0", this->config_.addr, 4) != 0) {
		*dp_options_ptr++ = DHCP_ROUTER;
		*dp_options_ptr++ = 4;
		memcpy((void*) dp_options_ptr, (void*) this->config_.addr, 4);
		dp_options_ptr += 4;
	}

	/* domain name */
	if (this->config_.domain != NULL) {
		int len = strlen(this->config_.domain);
		*dp_options_ptr++ = DHCP_DNSDOMAIN;
		*dp_options_ptr++ = len;
		memcpy((void*)dp_options_ptr, this->config_.domain, len);
		dp_options_ptr += len;
	}

	/* domain name server (DNS) */
	if ( memcmp("\0\0\0", this->config_.dns, 4) != 0) {
		*dp_options_ptr++ = DHCP_DNSSERVER;
		*dp_options_ptr++ = 4;
		memcpy((void*) dp_options_ptr, (void*) this->config_.dns, 4);
		dp_options_ptr += 4;
	}
	/* end */
	*dp_options_ptr++ = DHCP_END;
}



void DhcpServer::udp_recv_proc(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	static uint8_t* ptr = NULL;
	static dhcp_entry_t* entry = NULL;
	DhcpServer* dhcpServer_ptr = (DhcpServer*)arg;
	struct pbuf* pp = NULL;
	
	ptr = NULL;
	entry = NULL;
	
	uint16_t n = p->len;
	if (n > sizeof(dhcpServer_ptr->dhcp_data_)) n = sizeof(dhcpServer_ptr->dhcp_data_);
	memcpy(&dhcpServer_ptr->dhcp_data_, p->payload, n);
	switch (dhcpServer_ptr->dhcp_data_.dp_options[2])
	{
		case DHCP_DISCOVER:
			entry = dhcpServer_ptr->entry_by_mac(dhcpServer_ptr->dhcp_data_.dp_chaddr);
			if (entry == NULL) entry = dhcpServer_ptr->vacant_address();
			if (entry == NULL) break;

			dhcpServer_ptr->dhcp_data_.dp_op = 2; /* reply */
			dhcpServer_ptr->dhcp_data_.dp_secs = 0;
			dhcpServer_ptr->dhcp_data_.dp_flags = 0;
			memcpy(dhcpServer_ptr->dhcp_data_.dp_yiaddr, entry->addr, 4);
			memcpy(dhcpServer_ptr->dhcp_data_.dp_magic, DhcpServer::magicCookie_, 4);
			memset(dhcpServer_ptr->dhcp_data_.dp_options, 0, sizeof(dhcpServer_ptr->dhcp_data_.dp_options));

			dhcpServer_ptr->fill_options(DHCP_OFFER, entry);

			pp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcpServer_ptr->dhcp_data_), PBUF_RAM);
			if (pp == NULL) break;
			memcpy(pp->payload, &dhcpServer_ptr->dhcp_data_, sizeof(dhcpServer_ptr->dhcp_data_));
			udp_sendto_if(upcb, pp, IP_ADDR_BROADCAST, port, dhcpServer_ptr->netifStruct_);
			pbuf_free(pp);
			break;

		case DHCP_REQUEST:
			/* 1. find requested ipaddr in option list */
			ptr = dhcpServer_ptr->find_dhcp_option(dhcpServer_ptr->dhcp_data_.dp_options, sizeof(dhcpServer_ptr->dhcp_data_.dp_options), DHCP_IPADDRESS);
			if (ptr == NULL) break;
			if (ptr[1] != 4) break;
			ptr += 2;

			/* 2. does hw-address registered? */
			entry = dhcpServer_ptr->entry_by_mac(dhcpServer_ptr->dhcp_data_.dp_chaddr);
			if (entry != NULL) dhcpServer_ptr->free_entry(entry);

			/* 3. find requested ipaddr */
			entry = dhcpServer_ptr->entry_by_ip(ptr);
			if (entry == NULL) break;
			if (!dhcpServer_ptr->is_vacant(entry)) break;

			/* 4. fill struct fields */
			memcpy(dhcpServer_ptr->dhcp_data_.dp_yiaddr, ptr, 4);
			dhcpServer_ptr->dhcp_data_.dp_op = 2; /* reply */
			dhcpServer_ptr->dhcp_data_.dp_secs = 0;
			dhcpServer_ptr->dhcp_data_.dp_flags = 0;
			memcpy(dhcpServer_ptr->dhcp_data_.dp_magic, DhcpServer::magicCookie_, 4);

			/* 5. fill options */
			memset(dhcpServer_ptr->dhcp_data_.dp_options, 0, sizeof(dhcpServer_ptr->dhcp_data_.dp_options));

			dhcpServer_ptr->fill_options(DHCP_ACK, entry);

			/* 6. send ACK */
			pp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcpServer_ptr->dhcp_data_), PBUF_RAM);
			if (pp == NULL) break;
			memcpy(entry->mac, dhcpServer_ptr->dhcp_data_.dp_chaddr, 6);
			memcpy(pp->payload, &dhcpServer_ptr->dhcp_data_, sizeof(dhcpServer_ptr->dhcp_data_));
			udp_sendto_if(upcb, pp, IP_ADDR_BROADCAST, port, dhcpServer_ptr->netifStruct_);
			pbuf_free(pp);
			break;

			case DHCP_INFORM:
				
			/* 1. find requested ipaddr in option list */
			ptr = dhcpServer_ptr->dhcp_data_.dp_ciaddr;

			/* 2. does hw-address registered? */
			entry = dhcpServer_ptr->entry_by_mac(dhcpServer_ptr->dhcp_data_.dp_chaddr);
			if (entry != NULL) dhcpServer_ptr->free_entry(entry);

			/* 3. find requested ipaddr */
			entry = dhcpServer_ptr->entry_by_ip(ptr);
			if (entry == NULL) break;
			if (!dhcpServer_ptr->is_vacant(entry)) break;

			/* 4. fill struct fields */
			memcpy(dhcpServer_ptr->dhcp_data_.dp_yiaddr, ptr, 4);
			dhcpServer_ptr->dhcp_data_.dp_op = 2; /* reply */
			dhcpServer_ptr->dhcp_data_.dp_secs = 0;
			dhcpServer_ptr->dhcp_data_.dp_flags = 0;
			memcpy(dhcpServer_ptr->dhcp_data_.dp_magic, DhcpServer::magicCookie_, 4);

			/* 5. fill options */
			memset(dhcpServer_ptr->dhcp_data_.dp_options, 0, sizeof(dhcpServer_ptr->dhcp_data_.dp_options));

			dhcpServer_ptr->fill_options(DHCP_ACK, entry);

			memset(dhcpServer_ptr->dhcp_data_.dp_ciaddr,0,4);
			/* 6. send ACK */
			pp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcpServer_ptr->dhcp_data_), PBUF_RAM);
			if (pp == NULL) break;
			memcpy(entry->mac, dhcpServer_ptr->dhcp_data_.dp_chaddr, 6);
			memcpy(pp->payload, &dhcpServer_ptr->dhcp_data_, sizeof(dhcpServer_ptr->dhcp_data_));
			udp_sendto_if(upcb, pp, IP_ADDR_BROADCAST, port, dhcpServer_ptr->netifStruct_);
			pbuf_free(pp);
			break;
	
		default:
				break;
	}
	pbuf_free(p);
}

}
