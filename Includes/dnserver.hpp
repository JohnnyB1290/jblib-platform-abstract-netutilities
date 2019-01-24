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
 * brief:   tiny dns ipv4 server using lwip (pcb)
 */

#ifndef DNSERVER_HPP
#define DNSERVER_HPP

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "lwip/udp.h"

typedef struct dns_query
{
	char name[DNS_MAX_HOST_NAME_LEN];
	uint16_t type;
	uint16_t Class;
} dns_query_t;


class DNS_server_t
{
public:
	DNS_server_t(struct netif* LWIP_netif_ptr);
	~DNS_server_t(void);
private:
	int parse_next_query(void *data, int size, dns_query_t *query);
	static void udp_recv_proc(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
	struct udp_pcb* pcb;
	struct netif* LWIP_netif_ptr;
};

#endif
