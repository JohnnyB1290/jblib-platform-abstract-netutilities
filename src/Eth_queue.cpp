/*
 * Eth_queue.cpp
 *
 *  Created on: 18.10.2017 ã.
 *      Author: Stalker1290
 */
#include "Eth_queue.hpp"

Eth_queue_t::Eth_queue_t(uint16_t queue_size)
{
	this->Initialized = 0;
	this->queue_size = queue_size;
	if(this->queue_size == 0) return;
	this->Frames_buf_ptr = (EthernetFrame*)malloc_s(sizeof(EthernetFrame)*queue_size);
	this->frame_size_buf_ptr = (uint16_t*)malloc_s(sizeof(uint16_t)*queue_size);
	if((this->Frames_buf_ptr == (EthernetFrame*)NULL) || (this->frame_size_buf_ptr == (uint16_t*)NULL)) return;
	this->bw = 0;
	this->br = 0;
	this->treshold = 0;
	this->Initialized = 1;
}

Eth_queue_t::~Eth_queue_t(void)
{
	this->Initialized = 0;
	free(this->Frames_buf_ptr);
	free(this->frame_size_buf_ptr);
}


void Eth_queue_t::Add_Frame(EthernetFrame* NewFrame, uint16_t size)
{
	if(this->Initialized)
	{
		memcpy(this->Frames_buf_ptr[this->bw],NewFrame,size);
		this->frame_size_buf_ptr[this->bw] = size;
		this->bw++;
		if(this->bw == this->queue_size) this->bw = 0;
	}
}

uint16_t Eth_queue_t::Pull_out_Frame(EthernetFrame* Frame)
{
	uint16_t count;
	int16_t size;

	if(this->Initialized)
	{
		count = (this->bw >= this->br ? ((this->bw - this->br) % this->queue_size) : (this->queue_size - this->br + this->bw));
		if(count == 0) return 0;
		else
		{
			size = this->frame_size_buf_ptr[this->br];
			memcpy(Frame,&(this->Frames_buf_ptr[this->br]),size);
			this->br++;
			if(this->br == this->queue_size) this->br = 0;
			return size;
		}
	}
	else return 0;
}

uint16_t Eth_queue_t::Get_num_frames_in_queue(void)
{
	uint16_t count;
	if(this->Initialized)
	{
		count = (this->bw>=this->br ? ((this->bw - this->br) % this->queue_size) : (this->queue_size - this->br + this->bw));
		return count;
	}
	else return 0;
}

uint16_t Eth_queue_t::Get_free_frames_in_queue(void)
{
	uint16_t count;
	if(this->Initialized)
	{
		count = (this->bw>=this->br ? ((this->bw - this->br) % this->queue_size) : (this->queue_size - this->br + this->bw));
		return (this->queue_size-1-count);
	}
	else return 0;
}

void Eth_queue_t::Set_treshold(uint16_t treshold)
{
	this->treshold = treshold;
}

uint8_t Eth_queue_t::Check_treshold(void)
{
	uint16_t count;
	if(this->Initialized)
	{
		count = this->Get_num_frames_in_queue();
		if(count >= (this->treshold)) return 1;
		else return 0;
	}
	else return 0;
}


