/*
 * TCP_server.cpp
 *
 *  Created on: 31.10.2017
 *      Author: Stalker1290
 */

#include "TCP_server.hpp"
#include "lwip/tcp.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


TCP_server_t::TCP_server_t(uint8_t* SRC_IP, uint16_t SRC_PORT):void_channel_t()
{
	this->Constructor(SRC_IP,SRC_PORT);
}

TCP_server_t::TCP_server_t(uint16_t SRC_PORT):void_channel_t()
{
	this->Constructor((uint8_t*)NULL,SRC_PORT);
}

void TCP_server_t::Constructor(uint8_t* SRC_IP, uint16_t SRC_PORT)
{
	this->connections_counter = 0;
	for(uint32_t i = 0; i<TCP_Server_max_num_broadcast_connections; i++)
	{
		this->connections_ss_buf[i] = NULL;
	}
	if(SRC_IP != (uint8_t*)NULL) IP4_ADDR(&this->src_ipaddr, SRC_IP[0], SRC_IP[1], SRC_IP[2], SRC_IP[3]);
	else this->src_ipaddr = ip_addr_any;

	this->SRC_PORT = SRC_PORT;

	this->main_pcb = tcp_new();
	if (this->main_pcb != NULL)
	{
		err_t err;
		err = tcp_bind(this->main_pcb, &this->src_ipaddr, this->SRC_PORT);
		if (err == ERR_OK)
		{
			this->main_pcb = tcp_listen(this->main_pcb);
			tcp_arg(this->main_pcb, (void*)this);
			tcp_accept(this->main_pcb, TCP_server_t::Accept);
		}
		else
		{
			#ifdef USE_CONSOLE
			#ifdef TCP_server_CONSOLE
			printf("TCP Server on port %i Error! TCP Bind %i Error! \n\r",this->SRC_PORT,err);
			#endif
			#endif
		}
	}
	else
	{
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i Error! Main PCB = NULL! \n\r",this->SRC_PORT);
		#endif
		#endif
	}
}


err_t TCP_server_t::Accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	err_t ret_err;
	tcp_server_state_t* ss;

	LWIP_UNUSED_ARG(err);

  /* Unless this pcb should have NORMAL priority, set its priority now.
     When running out of pcbs, low priority pcbs can be aborted to create
     new pcbs of higher priority. */
	tcp_setprio(newpcb, TCP_PRIO_NORMAL);

	ss = (tcp_server_state_t*)malloc_s(sizeof(tcp_server_state_t));
	if (ss != NULL)
	{
		ss->state = ES_ACCEPTED;
		ss->pcb = newpcb;
		ss->p = NULL;
		ss->TCP_server_ptr = (TCP_server_t*)arg;

		tcp_arg(newpcb, ss);
		tcp_err(newpcb, TCP_server_t::Error);
		tcp_recv(newpcb, TCP_server_t::Recv);
		tcp_poll(newpcb, TCP_server_t::Poll, 0);

		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server: New connection accepted on Port %i! \n\r",ss->TCP_server_ptr->SRC_PORT);
		#endif
		#endif
		ss->TCP_server_ptr->Add_to_con_buf((void*)ss);
		ret_err = ERR_OK;
	}
	else
	{
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i Error! Accept malloc ss! \n\r",ss->TCP_server_ptr->SRC_PORT);
		#endif
		#endif
		ret_err = ERR_MEM;
	}
	return ret_err;
}


err_t TCP_server_t::Recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	tcp_server_state_t* ss;
	err_t ret_err;
	struct pbuf* p_next_ptr;

	LWIP_ASSERT("arg != NULL",arg != NULL);

	ss = (tcp_server_state_t*)arg;
	if (p == NULL)
	{
		/* remote host closed connection */
		ss->state = ES_CLOSING;
		if(ss->p == NULL)
		{
			#ifdef USE_CONSOLE
			#ifdef TCP_server_CONSOLE
			printf("TCP Server: Remote host closed connection on port %i! Sending done \n\r"
					,ss->TCP_server_ptr->SRC_PORT);
			#endif
			#endif
			/* we're done sending, close it */
			TCP_server_t::Close(tpcb, (void*)ss);
		}
		else
		{
			#ifdef USE_CONSOLE
			#ifdef TCP_server_CONSOLE
			printf("TCP Server: Remote host closed connection on port %i! Sending NOT done yet \n\r",
					ss->TCP_server_ptr->SRC_PORT);
			#endif
			#endif
			/* we're not done yet */
			tcp_sent(tpcb, TCP_server_t::Sent);
			ss->TCP_server_ptr->Send(ss);
		}
		ret_err = ERR_OK;
	}
	else if(err != ERR_OK)
	{
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i: Recv error %i ! \n\r",ss->TCP_server_ptr->SRC_PORT, err);
		#endif
		#endif
		/* cleanup, for unkown reason */
		if (p != NULL)
		{
			pbuf_free(p);
		}
		ret_err = err;
	}
	else if(ss->state == ES_ACCEPTED)
	{
		p_next_ptr = p;
		while(p_next_ptr != NULL)
		{
			if(ss->TCP_server_ptr->call_interface_ptr != (Channel_Call_Interface_t*)NULL)
			{
				ss->TCP_server_ptr->call_interface_ptr->
				channel_callback((uint8_t*)p_next_ptr->payload,p_next_ptr->len,(void*)ss->TCP_server_ptr,arg);
			}
			p_next_ptr = p_next_ptr->next;
		}
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	else if(ss->state == ES_CLOSING)
	{
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i: Error: remote side closing twice, trash data! \n\r",ss->TCP_server_ptr->SRC_PORT);
		#endif
		#endif
		/* odd case, remote side closing twice, trash data */
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	else
	{
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i: Error unkown ss->state, trash data! \n\r",ss->TCP_server_ptr->SRC_PORT);
		#endif
		#endif
		/* unkown es->state, trash data  */
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	return ret_err;
}

err_t TCP_server_t::Poll(void *arg, struct tcp_pcb *tpcb)
{
	err_t ret_err;
	tcp_server_state_t* ss;

	ss = (tcp_server_state_t*)arg;
	if (ss != NULL)
	{
		if (ss->p != NULL)
		{
			tcp_sent(tpcb, TCP_server_t::Sent);
			ss->TCP_server_ptr->Send(ss);
		}
		else
		{
			/* no remaining pbuf (chain)  */
			if(ss->state == ES_CLOSING)
			{
				#ifdef USE_CONSOLE
				#ifdef TCP_server_CONSOLE
				printf("TCP Server on port %i: POLL Closing\n\r",ss->TCP_server_ptr->SRC_PORT);
				#endif
				#endif
				TCP_server_t::Close(tpcb, (void*)ss);
			}
		}
		ret_err = ERR_OK;
	}
	else
	{
		/* nothing to be done */
		tcp_abort(tpcb);
		ret_err = ERR_ABRT;
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i: Error POLL ss == NULL!\n\r",ss->TCP_server_ptr->SRC_PORT);
		#endif
		#endif
		}
	return ret_err;
}


err_t TCP_server_t::Sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	tcp_server_state_t* ss;
	
	LWIP_UNUSED_ARG(len);

	ss = (tcp_server_state_t*)arg;

	if(ss->p != NULL)
	{
		/* still got pbufs to send */
		tcp_sent(tpcb, TCP_server_t::Sent);
		ss->TCP_server_ptr->Send(ss);
	}
	else
	{
		/* no more pbufs to send */
		if(ss->state == ES_CLOSING)
		{
			TCP_server_t::Close(tpcb, ss);
		}
	}
	
	return ERR_OK;
}

void TCP_server_t::Send(void* arg)
{
	tcp_server_state_t* ss;
	struct pbuf *ptr;
	err_t wr_err = ERR_OK;
	ss = (tcp_server_state_t*)arg;
	
	while ((wr_err == ERR_OK) &&(ss->p != NULL) &&(ss->p->len <= tcp_sndbuf(ss->pcb)))
	{
		ptr = ss->p;

		/* enqueue data for transmission */
		wr_err = tcp_write(ss->pcb, ptr->payload, ptr->len, 1);
		if (wr_err == ERR_OK)
		{
			u8_t freed;

			/* continue with next pbuf in chain (if any) */
			ss->p = ptr->next;
			if(ss->p != NULL)
			{
				/* new reference! */
				pbuf_ref(ss->p);
			}
			/* chop first pbuf from chain */
			do
			{
				/* try hard to free pbuf */
				freed = pbuf_free(ptr);
			}
			while(freed == 0);
		}
		else if(wr_err == ERR_MEM)
		{
			#ifdef USE_CONSOLE
			#ifdef TCP_server_CONSOLE
			printf("TCP Server on port %i: Error Send NO MEM! Try later\n\r",ss->TCP_server_ptr->SRC_PORT);
			#endif
			#endif
			/* we are low on memory, try later / harder, defer to poll */
			ss->p = ptr;
		}
		else
		{
			#ifdef USE_CONSOLE
			#ifdef TCP_server_CONSOLE
			printf("TCP Server on port %i: Error Send Undefined! \n\r",ss->TCP_server_ptr->SRC_PORT);
			#endif
			#endif
			/* other problem ?? */
		}
	}
	
}

void TCP_server_t::Tx(uint8_t *mes,uint16_t m_size,void* param)
{
	uint32_t unbuf_size = m_size;
	uint32_t payload_size = 0;
	uint8_t* mes_ptr = mes;
	struct pbuf* p = NULL;
	tcp_server_state_t* ss;

	if(this->connections_counter == 0) return;

	if(param == NULL)
	{
		for(uint32_t i = 0; i < this->connections_counter; i++)
		{
			if(this->connections_ss_buf[i] != NULL) this->Tx(mes, m_size, this->connections_ss_buf[i]);
		}
		return;
	}

	ss = (tcp_server_state_t*)param;

	if(ss->p != NULL) return;
	
	while(unbuf_size>0)
	{
		payload_size = MIN(unbuf_size,TCP_MSS);
		p = pbuf_alloc(PBUF_TRANSPORT, payload_size, PBUF_RAM);
		if (p == NULL) break;

		memcpy(p->payload,mes_ptr, payload_size);
		mes_ptr = mes_ptr + payload_size*sizeof(uint8_t);

		if(ss->p == NULL) ss->p = p;
		else
		{
			pbuf_chain(ss->p,p);
		}


		unbuf_size = unbuf_size - payload_size;
	}
	this->Send(param);
}

void TCP_server_t::Error(void *arg, err_t err)
{
	tcp_server_state_t* ss;

	ss = (tcp_server_state_t*)arg;
	#ifdef USE_CONSOLE
	#ifdef TCP_server_CONSOLE
	printf("TCP Server on port %i Error! Error %i! \n\r",ss->TCP_server_ptr->SRC_PORT,err);
	#else
	LWIP_UNUSED_ARG(err);
	#endif
	#else
	LWIP_UNUSED_ARG(err);
	#endif

	if (ss != NULL)
	{
		free_s(ss);
	}
}

void TCP_server_t::Close(struct tcp_pcb *tpcb, void* arg)
{
	tcp_server_state_t* ss;

	ss = (tcp_server_state_t*)arg;

	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);
	if (arg != NULL)
	{
		ss->TCP_server_ptr->Delete_from_con_buf(arg);
		free_s(arg);
	}
	else
	{
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i Error: Close ss = NULL connection! \n\r",
				ss->TCP_server_ptr->SRC_PORT);
		#endif
		#endif
	}
	tcp_close(tpcb);
}

TCP_server_t::~TCP_server_t(void)
{
	tcp_abort(this->main_pcb);
}

void TCP_server_t::Initialize(void* (*mem_alloc)(size_t),uint16_t tx_buf_size, Channel_Call_Interface_t* call_interface_ptr)
{
	this->call_interface_ptr = call_interface_ptr;
}

void TCP_server_t::DEInitialize(void)
{
	this->call_interface_ptr = (Channel_Call_Interface_t*)NULL;
}

void TCP_server_t::GetParameter(uint8_t ParamName, void* ParamValue)
{

}

void TCP_server_t::SetParameter(uint8_t ParamName, void* ParamValue)
{

}

void TCP_server_t::Add_to_con_buf(void* ss)
{
	if(this->connections_counter == TCP_Server_max_num_broadcast_connections)
	{
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i: Max number of broadcast connections achieved = %i!  \n\r",this->SRC_PORT,
				this->connections_counter);
		#endif
		#endif
	}
	else
	{
		for(int i = 0; i < TCP_Server_max_num_broadcast_connections; i++)
		{
			if(this->connections_ss_buf[i] == ss) break;
			if(this->connections_ss_buf[i] == NULL)
			{
				this->connections_ss_buf[i] = ss;
				break;
			}
		}
		this->connections_counter++;
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i: Number of connections = %i!  \n\r",this->SRC_PORT,
				this->connections_counter);
		#endif
		#endif
	}
}

void TCP_server_t::Delete_from_con_buf(void* ss)
{
	uint32_t index = 0;

	if(this->connections_counter == 0)
	{
		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i Error: Delete from connection buffer, but counter = 0! \n\r",this->SRC_PORT);
		#endif
		#endif

		return;
	}
	else
	{
		for(int i = 0; i < TCP_Server_max_num_broadcast_connections; i++)
		{
			if(this->connections_ss_buf[i] == ss) break;
			else index++;
		}
		if(index == (TCP_Server_max_num_broadcast_connections-1))
		{
			if(this->connections_ss_buf[index] == ss) this->connections_ss_buf[index] = NULL;
		}
		else
		{
			for(int i = index; i < (TCP_Server_max_num_broadcast_connections-1); i++)
			{
				this->connections_ss_buf[i] = this->connections_ss_buf[i+1];
				if(this->connections_ss_buf[i+1] == NULL) break;
			}
		}

		this->connections_counter--;

		#ifdef USE_CONSOLE
		#ifdef TCP_server_CONSOLE
		printf("TCP Server on port %i: Number of connections = %i!  \n\r",this->SRC_PORT,
				this->connections_counter);
		#endif
		#endif
	}
}
