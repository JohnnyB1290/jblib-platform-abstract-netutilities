/*
 * JB_Arp.cpp
 *
 *  Created on: 19.10.2017 ã.
 *      Author: Stalker1290
 */

#include "JB_Arp.hpp"
#include "CONTROLLER.hpp"


Arp_updater_t::Arp_updater_t(Ethernet_t* Eth_adapter_ptr):Ethernet_listener_t(), Callback_Interface_t()
{
	this->Eth_adapter_ptr = Eth_adapter_ptr;
	this->Eth_adapter_ptr->GetParameter(MAC_param, (void*)&(this->MAC));
	memset(&(this->ip_table),0,sizeof(IP_table_for_arp_t));
	memset(&(this->arp_table),0,sizeof(arp_table_t));
	CONTROLLER_t::get_Time_Engine()->NRT_setEvent(Lowest_priority_delay, 1000000, this, NULL);
}

Arp_updater_t::~Arp_updater_t(void)
{

}

arp_table_t* Arp_updater_t::Get_arp_table_ptr(void)
{
	return &this->arp_table;
}

void Arp_updater_t::create_etx_arp_reply(uint8_t* requBuff, uint8_t* outBuff, uint8_t* S_MAC, uint16_t* frame_size)
{
		*frame_size = ETX_ETH_HEADER_LEN + ETX_ARP_BODY_LEN;
		memcpy(outBuff+ETX_ETH_D_MAC_OFFSET,requBuff+ETX_ARP_SENDER_MAC_OFFSET,ETX_HW_SIZE);
		memcpy(outBuff+ETX_ETH_S_MAC_OFFSET,S_MAC,ETX_HW_SIZE);
		outBuff[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
		outBuff[ETX_ETH_ETHER_TYPE_OFFSET+1] = 0x06; // ARP

		outBuff[ETX_ARP_HW_TYPE_OFFSET] = 0x00;
		outBuff[ETX_ARP_HW_TYPE_OFFSET+1] = 0x01; // ETHERNET
		outBuff[ETX_ARP_PROTO_TYPE_OFFSET] = 0x08;
		outBuff[ETX_ARP_PROTO_TYPE_OFFSET+1] = 0x00;//IP
		outBuff[ETX_ARP_HW_SIZE_OFFSET] = 0x06;
		outBuff[ETX_ARP_PROTO_SIZE_OFFSET] = 0x04;
		outBuff[ETX_ARP_OPCODE_OFFSET] = 0x00;//reply
		outBuff[ETX_ARP_OPCODE_OFFSET+1] = 0x02;//reply
		memcpy(outBuff+ETX_ARP_SENDER_MAC_OFFSET,S_MAC,ETX_HW_SIZE);
		memcpy(outBuff+ETX_ARP_SENDER_IP_OFFSET,requBuff+ETX_ARP_TARGET_IP_OFFSET,ETX_PROTO_SIZE);
		memcpy(outBuff+ETX_ARP_TARGET_MAC_OFFSET,requBuff+ETX_ARP_SENDER_MAC_OFFSET,ETX_HW_SIZE);
		memcpy(outBuff+ETX_ARP_TARGET_IP_OFFSET,requBuff+ETX_ARP_SENDER_IP_OFFSET,ETX_PROTO_SIZE);
}

void Arp_updater_t::create_etx_arp_request(const uint8_t* IP, uint8_t* S_MAC,uint8_t* S_IP, uint8_t* outBuff, uint16_t* frame_size)
{
		uint8_t Zero_Mac[6] = {0,0,0,0,0,0};
		uint8_t Broadcast_Mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

		*frame_size = ETX_ETH_HEADER_LEN + ETX_ARP_BODY_LEN;

		memcpy(outBuff+ETX_ETH_D_MAC_OFFSET,(uint8_t*)Broadcast_Mac,ETX_HW_SIZE);
		memcpy(outBuff+ETX_ETH_S_MAC_OFFSET,S_MAC,ETX_HW_SIZE);
		outBuff[ETX_ETH_ETHER_TYPE_OFFSET] = 0x08;
		outBuff[ETX_ETH_ETHER_TYPE_OFFSET+1] = 0x06; // ARP

		outBuff[ETX_ARP_HW_TYPE_OFFSET] = 0x00;
		outBuff[ETX_ARP_HW_TYPE_OFFSET+1] = 0x01; // ETHERNET
		outBuff[ETX_ARP_PROTO_TYPE_OFFSET] = 0x08;
		outBuff[ETX_ARP_PROTO_TYPE_OFFSET+1] = 0x00;//IP
		outBuff[ETX_ARP_HW_SIZE_OFFSET] = 0x06;
		outBuff[ETX_ARP_PROTO_SIZE_OFFSET] = 0x04;
		outBuff[ETX_ARP_OPCODE_OFFSET] = 0x00;//
		outBuff[ETX_ARP_OPCODE_OFFSET+1] = 0x01;//request
		memcpy(outBuff+ETX_ARP_SENDER_MAC_OFFSET,S_MAC,ETX_HW_SIZE);
		memcpy(outBuff+ETX_ARP_SENDER_IP_OFFSET,S_IP,ETX_PROTO_SIZE);
		memcpy(outBuff+ETX_ARP_TARGET_MAC_OFFSET,(uint8_t*)Zero_Mac,ETX_HW_SIZE);
		memcpy(outBuff+ETX_ARP_TARGET_IP_OFFSET,IP,ETX_PROTO_SIZE);
}

void Arp_updater_t::Parse_frame(EthernetFrame* frame_ptr,uint16_t frame_size, Ethernet_t* Eth_adapter_ptr, void* Parameter)
{
	static EthernetFrame* Out_Frame_ptr;
	static uint16_t* Out_frame_size_ptr;
	static uint8_t* In_Frame_ptr;
	int16_t arp_index;
	uint8_t allow_arp = 0;
	
	if(frame_size == 0) return;

	In_Frame_ptr = (uint8_t*)frame_ptr;

	if(Eth_utilities_t::etx_get_ether_type(In_Frame_ptr) == ETX_ETHER_TYPE_ARP) //ARP
	{
		if(Parameter == NULL)
		{
			if(this->Is_ip_in_ip_table_for_arp((uint8_t*)&(In_Frame_ptr[ETX_ARP_TARGET_IP_OFFSET]))) // TArget ip is one of MyIP(ip_table_for_arp)
			{
				allow_arp = 1;
			}
			else allow_arp = 0;
		}
		else allow_arp = 1;

		if(allow_arp)
		{
			if((In_Frame_ptr[ETX_ARP_OPCODE_OFFSET] == 0x00)&&(In_Frame_ptr[ETX_ARP_OPCODE_OFFSET+1] == 0x02))//reply
			{
				arp_index = this->get_index_arp_table_from_ip((uint8_t*)&(In_Frame_ptr[ETX_ARP_SENDER_IP_OFFSET]));// find arp records from table

				if(arp_index >= 0)//record find -> update
				{
					memcpy((uint8_t*)(this->arp_table.mac[arp_index]),&(In_Frame_ptr[ETX_ARP_SENDER_MAC_OFFSET]),ETX_HW_SIZE);
					this->arp_table.time_records[arp_index] = 0;
				}
				else // records not find -> create new record
				{
					arp_index = this->arp_table.total_ip_num;
					memcpy((uint8_t*)(this->arp_table.mac[arp_index]),&(In_Frame_ptr[ETX_ARP_SENDER_MAC_OFFSET]),ETX_HW_SIZE);
					memcpy((uint8_t*)(this->arp_table.ip[arp_index]),&(In_Frame_ptr[ETX_ARP_SENDER_IP_OFFSET]),ETX_PROTO_SIZE);
					this->arp_table.time_records[arp_index] = 0;
					this->arp_table.total_ip_num++;
					#ifdef Arp_console
					#ifdef USE_CONSOLE
					char* Adapter_name;
					this->Eth_adapter_ptr->GetParameter(name_param, (void*)&Adapter_name);
					printf("%s Arp reply. Create record for ip ",Adapter_name);
					printf("%i.%i.%i.%i\r\n\r\n", this->arp_table.ip[arp_index][0],this->arp_table.ip[arp_index][1],this->arp_table.ip[arp_index][2],this->arp_table.ip[arp_index][3]);
					#endif
					#endif
				}
			}
			else if((In_Frame_ptr[ETX_ARP_OPCODE_OFFSET] == 0x00)&&(In_Frame_ptr[ETX_ARP_OPCODE_OFFSET+1] == 0x01))//request
			{
				Out_Frame_ptr = (EthernetFrame*)malloc_s(sizeof(EthernetFrame));
				Out_frame_size_ptr = (uint16_t*)malloc_s(sizeof(uint16_t));
				if((Out_Frame_ptr == (EthernetFrame*)NULL) || (Out_frame_size_ptr == (uint16_t*)NULL)) return;

				this->create_etx_arp_reply(In_Frame_ptr, (uint8_t*)Out_Frame_ptr, this->MAC, Out_frame_size_ptr);
				this->Eth_adapter_ptr->Add_to_TX_queue(Out_Frame_ptr,*Out_frame_size_ptr);

				arp_index = this->get_index_arp_table_from_ip((uint8_t*)&(In_Frame_ptr[ETX_ARP_SENDER_IP_OFFSET]));// find arp records from table
				if(arp_index >= 0)//record find -> update
				{
					memcpy((uint8_t*)(this->arp_table.mac[arp_index]),&(In_Frame_ptr[ETX_ARP_SENDER_MAC_OFFSET]),ETX_HW_SIZE);
					this->arp_table.time_records[arp_index] = 0;
				}
				else // records not find -> create new record
				{
					arp_index = this->arp_table.total_ip_num;
					memcpy((uint8_t*)(this->arp_table.mac[arp_index]),&(In_Frame_ptr[ETX_ARP_SENDER_MAC_OFFSET]),ETX_HW_SIZE);
					memcpy((uint8_t*)(this->arp_table.ip[arp_index]),&(In_Frame_ptr[ETX_ARP_SENDER_IP_OFFSET]),ETX_PROTO_SIZE);
					this->arp_table.time_records[arp_index] = 0;
					this->arp_table.total_ip_num++;
					#ifdef Arp_console
					#ifdef USE_CONSOLE
					char* Adapter_name;
					this->Eth_adapter_ptr->GetParameter(name_param, (void*)&Adapter_name);
					printf("%s Arp reqest for ip ",Adapter_name);
					printf("%i.%i.%i.%i\r\n", In_Frame_ptr[ETX_ARP_TARGET_IP_OFFSET+0],In_Frame_ptr[ETX_ARP_TARGET_IP_OFFSET+1],In_Frame_ptr[ETX_ARP_TARGET_IP_OFFSET+2],In_Frame_ptr[ETX_ARP_TARGET_IP_OFFSET+3]);
					printf("Create record for ip ");
					printf("%i.%i.%i.%i\r\n\r\n", this->arp_table.ip[arp_index][0],this->arp_table.ip[arp_index][1],this->arp_table.ip[arp_index][2],this->arp_table.ip[arp_index][3]);
					#endif
					#endif
				}
				free_s(Out_Frame_ptr);
				free_s(Out_frame_size_ptr);
			}
		}
	}
}

void Arp_updater_t::Send_arp_request(uint8_t* IP_d)
{
	EthernetFrame* Out_Frame_ptr;
	uint16_t* Out_frame_size_ptr;
	static uint8_t* My_MAC_ptr;

	this->Eth_adapter_ptr->GetParameter(MAC_param,(void*)&My_MAC_ptr);
	Out_Frame_ptr = (EthernetFrame*)malloc_s(sizeof(EthernetFrame));
	Out_frame_size_ptr = (uint16_t*)malloc_s(sizeof(uint16_t));
	if((Out_Frame_ptr == (EthernetFrame*)NULL) || (Out_frame_size_ptr == (uint16_t*)NULL)) return;

	for(uint8_t i=0; i< this->ip_table.total_ip_num; i++)
	{
		this->create_etx_arp_request(IP_d, My_MAC_ptr,(uint8_t*)this->ip_table.ip[i], (uint8_t*)Out_Frame_ptr, Out_frame_size_ptr);
		this->Eth_adapter_ptr->Add_to_TX_queue(Out_Frame_ptr,*Out_frame_size_ptr);
	}

	free_s(Out_Frame_ptr);
	free_s(Out_frame_size_ptr);
}

void Arp_updater_t::Add_ip_in_ip_table_for_arp(uint8_t* IP)
{
	memcpy(this->ip_table.ip[this->ip_table.total_ip_num], IP, ETX_PROTO_SIZE);
	this->ip_table.total_ip_num++;
}

bool Arp_updater_t::Is_ip_in_ip_table_for_arp(uint8_t* IP)
{
	if(this->ip_table.total_ip_num == 0) return false;

	for(uint16_t i=0;i<(this->ip_table.total_ip_num);i++)
	{
		if((memcmp(IP,this->ip_table.ip[i],ETX_PROTO_SIZE) == 0)) return true;
	}
	return false;
}

/* return index of records mac_table with IP, or -1 if recods nof find*/
int16_t Arp_updater_t::get_index_arp_table_from_ip(uint8_t* IP)
{
	if(this->arp_table.total_ip_num == 0) return -1;

	for(int16_t index = 0; index<this->arp_table.total_ip_num; index++)
	{
		if(memcmp(IP,this->arp_table.ip[index],ETX_PROTO_SIZE)==0) return index;
	}
	return -1;
}

bool Arp_updater_t::Get_MAC_from_IP(uint8_t* IP, uint8_t* MAC)
{
	int16_t index;
	index = this->get_index_arp_table_from_ip(IP);
	if(index>=0)
	{
		memcpy(MAC, this->arp_table.mac[index],ETX_HW_SIZE);
		return true;
	}
	else return false;
}

bool Arp_updater_t::Get_IP_from_MAC(uint8_t* MAC, uint8_t* IP)
{
	if(this->arp_table.total_ip_num == 0) return false;

	for(uint16_t index = 0; index<this->arp_table.total_ip_num; index++)
	{
		if(memcmp(MAC,this->arp_table.mac[index],ETX_HW_SIZE)==0)
		{
			memcpy(IP, this->arp_table.ip[index],ETX_PROTO_SIZE);
			return true;
		}
	}
	return false;
}

void Arp_updater_t::void_callback(void* Intf_ptr, void* parameters)
{
	if(this->arp_table.total_ip_num > 0)
	{
		for(uint16_t i = 0; i<(this->arp_table.total_ip_num); i++)
		{
			this->arp_table.time_records[i]++;
			if(this->arp_table.time_records[i] > ETX_ARP_REFRESH_RECORDS_TIME_s)
			{
				this->Send_arp_request((uint8_t*)(this->arp_table.ip[i]));
			}
			if(this->arp_table.time_records[i] > ETX_ARP_DELETE_RECORDS_TIME_s)
			{
				#ifdef Arp_console
				#ifdef USE_CONSOLE
				char* Adapter_name;
				this->Eth_adapter_ptr->GetParameter(name_param, (void*)&Adapter_name);
				printf("%s Arp table delete record for ip ",Adapter_name);
				printf("%i.%i.%i.%i\r\n", this->arp_table.ip[i][0],this->arp_table.ip[i][1],this->arp_table.ip[i][2],this->arp_table.ip[i][3]);
				#endif
				#endif
				if(i != (this->arp_table.total_ip_num-1))
				{
					memmove((uint8_t*)(this->arp_table.ip[i]), (uint8_t*)(this->arp_table.ip[i+1]), sizeof(ETX_PROTO_SIZE)*(this->arp_table.total_ip_num - 1 - i));
				}
				this->arp_table.total_ip_num -- ;
			}
		}
	}
	CONTROLLER_t::get_Time_Engine()->NRT_setEvent(Lowest_priority_delay, 1000000, this, NULL);
}


