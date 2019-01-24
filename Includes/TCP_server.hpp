/*
 * TCP_server.hpp
 *
 *  Created on: 31.10.2017
 *      Author: Stalker1290
 */

#ifndef TCP_SERVER_HPP_
#define TCP_SERVER_HPP_

#include "Void_Channel.hpp"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

class TCP_server_t:public void_channel_t
{
public:
	TCP_server_t(uint8_t* SRC_IP, uint16_t SRC_PORT);
	TCP_server_t(uint16_t SRC_PORT);
	virtual ~TCP_server_t(void);
	virtual void Initialize(void* (*mem_alloc)(size_t),uint16_t tx_buf_size, Channel_Call_Interface_t* call_interface_ptr);
	virtual void DEInitialize(void);
	virtual void Tx(uint8_t *mes,uint16_t m_size,void* param);
	virtual void GetParameter(uint8_t ParamName, void* ParamValue);
	virtual void SetParameter(uint8_t ParamName, void* ParamValue);
	Channel_Call_Interface_t* call_interface_ptr;
	uint32_t connections_counter;
	void* connections_ss_buf[TCP_Server_max_num_broadcast_connections];
private:
	void Constructor(uint8_t* SRC_IP, uint16_t SRC_PORT);
	void Send(void* arg);
	static err_t Poll(void *arg, struct tcp_pcb *tpcb);
	static err_t Accept(void *arg, struct tcp_pcb *newpcb, err_t err);
	static void Error(void *arg, err_t err);
	static void Close(struct tcp_pcb *tpcb, void* arg);
	static err_t Recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
	static err_t Sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
	struct tcp_pcb* main_pcb;
	ip_addr_t src_ipaddr;
	uint16_t SRC_PORT;
	void Add_to_con_buf(void* ss);
	void Delete_from_con_buf(void* ss);
};

typedef enum
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_CLOSING
}tcp_server_states_enum_t;

typedef struct server_state_struct
{
  uint8_t state;
  struct tcp_pcb *pcb;
  /* pbuf (chain) to recycle */
  struct pbuf *p;
  TCP_server_t* TCP_server_ptr;
}tcp_server_state_t;



#endif /* TCP_SERVER_HPP_ */
