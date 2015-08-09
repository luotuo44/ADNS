//Filename: DnsTcpStateMachine.hpp
//Date: 2015-7-27

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DNSTCPSTATEMACHINE_HPP
#define DNSTCPSTATEMACHINE_HPP

#include<memory>
#include<string>
#include<map>

#include"typedef-internal.hpp"

namespace ADNS
{


class DnsTcpStateMachine
{
public:
    DnsTcpStateMachine(EventCreater &ev_creater);
    ~DnsTcpStateMachine();
    DnsTcpStateMachine(DnsTcpStateMachine &)=delete;
    DnsTcpStateMachine& operator = (DnsTcpStateMachine &)=delete;


    void setResCB(DnsExplorerResCB &cb);
    void addQuery(const DnsQuery_t &query);
    void addQuery(int id, const std::string &domain, const std::string &dns_server, DnsQueryCB cb);
    void eventCB(int fd, int events, void *arg);


private:
    struct QueryPacket;
    typedef std::shared_ptr<QueryPacket> QueryPacketPtr;

    void updateEvent(QueryPacketPtr &q, int events, int milliseconds =-1);
    void replyResult(QueryPacketPtr &q, bool success);
    int driveMachine(QueryPacketPtr &q);
    bool getDNSQueryPacket(QueryPacketPtr &query);
private:
    EventCreater m_ev_creater;
    DnsExplorerResCB m_res_cb;
    std::map<int, QueryPacketPtr> m_querys;

};


}

#endif // DNSTCPSTATEMACHINE_HPP
