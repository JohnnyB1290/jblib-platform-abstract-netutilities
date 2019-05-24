/**
 * @file
 * @brief Ethernet Queue class Realization
 *
 *
 * @note
 * Copyright © 2019 Evgeniy Ivanov. Contacts: <strelok1290@gmail.com>
 * All rights reserved.
 * @note
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * @note
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @note
 * This file is a part of JB_Lib.
 */

// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "string.h"
#include "EthernetQueue.hpp"

namespace jblib::ethutilities
{


EthernetQueue::EthernetQueue(uint16_t size)
{
	this->size_ = size;
	if(this->size_ == 0)
		return;
	this->frames_ = (EthernetFrame*)malloc_s(sizeof(EthernetFrame) * size);
	if(this->frames_ == NULL)
		return;
	this->frameSizes_ = (uint16_t*)malloc_s(sizeof(uint16_t) * size);
	if(this->frameSizes_ == NULL){
		free_s(this->frames_);
		return;
	}
	this->isInitialized_ = true;
}



EthernetQueue::~EthernetQueue(void)
{
	this->isInitialized_ = false;
	free_s(this->frames_);
	free_s(this->frameSizes_);
}



void EthernetQueue::push(EthernetFrame* frame, uint16_t size)
{
	if(this->isInitialized_) {
		memcpy(this->frames_[this->bw_], frame, size);
		this->frameSizes_[this->bw_] = size;
		this->bw_++;
		if(this->bw_ == this->size_)
			this->bw_ = 0;
	}
}



uint16_t EthernetQueue::pull(EthernetFrame* frame)
{
	if(this->isInitialized_) {
		uint16_t count = D_A_MIN_B_MOD_C(this->bw_ ,this->br_, this->size_);
		if(count){
			uint16_t size = this->frameSizes_[this->br_];
			memcpy(frame, &(this->frames_[this->br_]), size);
			this->br_++;
			if(this->br_ == this->size_)
				this->br_ = 0;
			return size;
		}
	}
	return 0;
}



uint16_t EthernetQueue::getCount(void) const
{
	if(this->isInitialized_)
		return D_A_MIN_B_MOD_C(this->bw_ ,this->br_, this->size_);
	else
		return 0;
}



uint16_t EthernetQueue::getFree(void) const
{
	if(this->isInitialized_)
		return (this->size_ - 1 -
				D_A_MIN_B_MOD_C(this->bw_ ,this->br_, this->size_));
	else
		return 0;
}



void EthernetQueue::setTreshold(uint16_t treshold)
{
	this->treshold_ = treshold;
}



bool EthernetQueue::isAboveTreshold(void)
{
	return ((this->isInitialized_) &&
			(D_A_MIN_B_MOD_C(this->bw_ ,this->br_, this->size_) >= (this->treshold_))) ?
					true : false;
}

}
