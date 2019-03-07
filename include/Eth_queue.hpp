/*
 * Eth_queue.hpp
 *
 *  Created on: 18.10.2017
 *      Author: Stalker1290
 */

#ifndef SRC_ETH_QUEUE_HPP_
#define SRC_ETH_QUEUE_HPP_

#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "chip.h"

class Eth_queue_t
{
public:
	Eth_queue_t(uint16_t queue_size);
	~Eth_queue_t(void);
	void Add_Frame(EthernetFrame* NewFrame, uint16_t size);
	uint16_t Pull_out_Frame(EthernetFrame* Frame);
	uint16_t Get_num_frames_in_queue(void);
	uint16_t Get_free_frames_in_queue(void);
	void Set_treshold(uint16_t treshold);
	uint8_t Check_treshold(void);
private:
	EthernetFrame* Frames_buf_ptr;
	uint16_t* frame_size_buf_ptr;
	uint16_t bw;
	uint16_t br;
	uint16_t treshold;
	uint16_t queue_size;
	uint8_t Initialized;
};

#endif /* SRC_ETH_QUEUE_HPP_ */
