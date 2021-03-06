/*
 * The MIT License (MIT)
 *
 * Copyright © 2015 by Sergey Fetisov <fsenok@gmail.com>
 * Copyright © 2020 Evgeniy Ivanov. Contacts: <strelok1290@gmail.com>
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

#ifndef DNS_SERVER_HPP
#define DNS_SERVER_HPP

#include "jbkernel/jb_common.h"
#include "jbkernel/callback_interfaces.hpp"
#include "lwip/udp.h"
#include <forward_list>

namespace jblib
{
namespace ethutilities
{

class DnsServer : public jblib::jbkernel::IVoidCallback
{
public:
    static DnsServer* getDnsServer();
    void start();
    void stop();
    void addHost(const char* hostName, bool redirectRequest = false);
    void deleteHost(const char* hostName);

private:
    #pragma pack(push, 1)
    typedef struct
    {
        char name[CONFIG_JBLIB_DNS_SERVER_HOST_NAME_MAX_SIZE];
        uint16_t type;
        uint16_t Class;
    } dns_query_t;
    #pragma pack(pop)

    struct DnsHostRecord
    {
        std::string name = "";
        bool redirectRequest = false;
    };

    static constexpr const char* logTag_ = "[ DNS Server ]";
    static DnsServer* dnsServer_;

    bool isStarted_ = false;
    int socket_ = -1;
    uint32_t replyIp_ = 0;
    std::forward_list<DnsHostRecord> hostsList_;
    xSemaphoreHandle hostsListMutex_ = xSemaphoreCreateMutex();

    DnsServer();
    int parseNextQuery(void *data, int size, dns_query_t *query);
    void voidCallback(void* source, void* parameter) override;
};

}
}

#endif /* DNS_SERVER_HPP */
