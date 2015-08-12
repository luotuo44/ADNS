//Filename:
//Date: 2015-8-11

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DNSUDPSTATEMACHINE
#define DNSUDPSTATEMACHINE

#include<map>
#include<memory>

#include"typedef.hpp"
#include"typedef-internal.hpp"



namespace ADNS
{

class DnsUdpStateMachine
{
public:
    DnsUdpStateMachine(EventCreater &ev_creater);
    ~DnsUdpStateMachine();

    DnsUdpStateMachine(const DnsUdpStateMachine& )=delete;
    DnsUdpStateMachine& operator = (const DnsUdpStateMachine& )=delete;

    void setResCB(DnsExplorerResCB &cb);
    void addQuery(const DnsQuery_t &query);
    void eventCB(int fd, int events, void *arg);

private:
    struct QueryPacket;
    using QueryPacketPtr = std::shared_ptr<QueryPacket>;

private:
    void updateEvent(QueryPacketPtr &q, int events, int milliseconds =-1);
    void replyResult(QueryPacketPtr &q, bool success);
    bool getDNSQueryPacket(QueryPacketPtr &query);


protected:
    EventCreater m_ev_creater;
    DnsExplorerResCB m_res_cb;
    std::map<int, QueryPacketPtr> m_querys;
};


}


#endif // DNSUDPSTATEMACHINE

