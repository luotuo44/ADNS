//Filename: DnsExplorer.hpp
//Date: 2015-7-16

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef SMARTDNS_HPP
#define SMARTDNS_HPP


#include<list>
#include<memory>

#include"MutexLock.hpp"
#include"typedef.hpp"
#include"typedef-internal.hpp"
#include"MinUnusedID.hpp"

namespace ADNS
{

class Event;
typedef std::shared_ptr<Event> EventPtr;

class DnsTcpStateMachine;
using DnsTcpStateMachinePtr = std::shared_ptr<DnsTcpStateMachine>;
class DnsUdpStateMachine;
using DnsUdpStateMachinePtr = std::shared_ptr<DnsUdpStateMachine>;

class DnsExplorer
{
public:
    DnsExplorer(EventCreater &ev_creater);
    ~DnsExplorer();

    DnsExplorer(const DnsExplorer&)=delete;
    DnsExplorer& operator= (const DnsExplorer&)=delete;

    void init();

    void addDnsRequest(DnsQuery_t &&query);
    void pipe_cb(int fd, int events, void *arg);


    void setDnsClientResCB(DnsClientResCB cb);

    void DnsResultCB(DnsQueryResult_t &&res);

private:
    void updateEvent(int fd, int new_events, int milliseconds =-1);
    bool handleNewRequest(DnsQuery_t &&query);
    void restoreRequest(DnsQuery_t &&query);

private:
    EventCreater m_ev_creater;
    int m_pipe[2];
    Net::EventPtr m_pipe_ev;

    DnsClientResCB m_client_res_cb;
    std::list<DnsQuery_t> m_requests;
    Mutex m_request_mutex;

    DnsTcpStateMachinePtr m_tcp_machine;
    DnsUdpStateMachinePtr m_udp_machine;
    Utility::MinUnusedID m_minUnusedID;
};


typedef std::shared_ptr<DnsExplorer> DnsExplorerPtr;
typedef std::weak_ptr<DnsExplorer> DnsExplorerWPtr;


}

#endif // SMARTDNS_HPP
