/**
 * @file
 * @brief Ethernet Queue class Description
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

#ifndef ETHERNET_QUEUE_HPP_
#define ETHERNET_QUEUE_HPP_

#include "jb_common.h"

namespace jblib
{
namespace ethutilities
{

class EthernetQueue
{
public:
	EthernetQueue(uint16_t size);
	~EthernetQueue(void);
	void push(EthernetFrame* frame, uint16_t size);
	uint16_t pull(EthernetFrame* frame);
	uint16_t getCount(void) const;
	uint16_t getFree(void) const;
	void setTreshold(uint16_t treshold);
	bool isAboveTreshold(void);

private:
	EthernetFrame* frames_ = NULL;
	uint16_t* frameSizes_ = NULL;
	uint16_t bw_ = 0;
	uint16_t br_ = 0;
	uint16_t treshold_ = 0;
	uint16_t size_ = 0;
	bool isInitialized_ = false;
};

}
}

#endif /* ETHERNET_QUEUE_HPP_ */
