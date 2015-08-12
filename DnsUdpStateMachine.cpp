//Filename:
//Date: 2015-8-11

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"DnsUdpStateMachine.hpp"


#include<assert.h>

#include<iostream>
#include<string>
#include<memory>

#include"Reactor.hpp"
#include"SocketOps.hpp"
#include"Logger.hpp"
#include"DNS.hpp"

namespace ADNS
{


struct DnsUdpStateMachine::QueryPacket
{
    int id;
    int fd;
    Net::EventPtr ev;
    std::string domain;
    std::string dns_server;//ip
    DNSQueryType query_type;
    DnsQueryCB cb;
    uint32_t ttl;

    RecordData ips;//dns query result

    std::string r_buff;
    std::string w_buff;

    std::string fail_reason;
};

DnsUdpStateMachine::DnsUdpStateMachine(EventCreater &ev_creater)
    : m_ev_creater(ev_creater)
{

}


DnsUdpStateMachine::~DnsUdpStateMachine()
{

}



void DnsUdpStateMachine::setResCB(DnsExplorerResCB &cb)
{
    assert(cb != nullptr);
    m_res_cb = cb;
}


static std::string getError(const char* str)
{
    char buf[200];
    snprintf(buf, sizeof(buf), "%m : %s ", str);
    return std::string(buf);
}


void DnsUdpStateMachine::addQuery(const DnsQuery_t &query)
{
    QueryPacketPtr q = std::make_shared<QueryPacket>();
    q->id = query.id;
    q->domain = query.domain;
    q->dns_server = query.dns_server;

    q->query_type = query.query_type;
    q->cb = query.cb;

    do
    {
        q->fd = Net::SocketOps::new_udp_socket();
        if( q->fd == -1)
        {
            q->fail_reason = getError("new_udp_socket fail ");
            break;
        }

        if( !getDNSQueryPacket(q) )
        {
            q->fail_reason = "hostname format error ";
            break;
        }

        int ret = Net::SocketOps::udp_write(q->fd, q->w_buff.c_str(), q->w_buff.size(), q->dns_server.c_str());

        if( ret < 0 || static_cast<size_t>(ret) < q->w_buff.size())
        {
            q->fail_reason = getError("write date to dns server fail ");
            break;
        }

        updateEvent(q, EV_READ|EV_PERSIST|EV_TIMEOUT, 20000);//wait for 20 seconds
        m_querys[q->fd] = q;
        return ;
    }while(0);


    LOG(Log::ERROR)<<q->fail_reason<<" dns server : "<<q->dns_server<<" for query "<<q->domain;
    replyResult(q, false);
    return ;
}


void DnsUdpStateMachine::replyResult(QueryPacketPtr &q, bool success)
{
    assert(m_res_cb != nullptr);

    DnsQueryResult_t res;
    res.id = q->id;
    res.success = success;
    res.fail_reason = std::move(q->fail_reason);
    res.domain = std::move(q->domain);
    res.dns_server = std::move(q->dns_server);
    res.record_data = std::move(q->ips);
    res.query_type = q->query_type;
    res.cb = q->cb;
    res.ttl = q->ttl;
    //res.cb should set by DnsExplorer

    m_res_cb(std::move(res));
}


void DnsUdpStateMachine::updateEvent(QueryPacketPtr &query, int new_events, int milliseconds)
{
    Net::Reactor::delEvent(query->ev);
    query->ev = m_ev_creater(query->fd, new_events, std::bind(&DnsUdpStateMachine::eventCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), nullptr);
    Net::Reactor::addEvent(query->ev, milliseconds);
}


void DnsUdpStateMachine::eventCB(int fd, int events, void *arg)
{
    auto it = m_querys.find(fd);
    if( it == m_querys.end() )
    {
        std::cout<<"event update a fd that does exist"<<std::endl;
        return ;
    }

    QueryPacketPtr q = it->second;
    bool success = false;
    do
    {
        if( events & EV_TIMEOUT )
        {
            q->fail_reason = "timeout to wait for dns_server ";
            break;
        }

        assert(events & EV_READ);
        q->r_buff.resize(512);
        int ret = Net::SocketOps::udp_read(q->fd, &q->r_buff[0], q->r_buff.size());
        if( ret <= 0)
        {
            q->fail_reason = getError("read from dns server fail ");
            break;
        }

        q->ips = DNS::parseDNSResultPacket(q->r_buff.c_str(), ret, q->ttl);
        if( q->ips.empty() )
        {
             q->fail_reason = "fail to parse dns server response packet";
             break;
        }

        success = true;
    }while(0);

    if( !success )
        LOG(Log::ERROR)<<q->fail_reason<<q->dns_server<<" for query "<<q->domain;


    Net::Reactor::delEvent(q->ev);
    Net::SocketOps::close_socket(q->fd);
    m_querys.erase(it);

    replyResult(q, success);

    (void)arg;
}



bool DnsUdpStateMachine::getDNSQueryPacket(QueryPacketPtr &query)
{
    query->w_buff = DNS::getDNSPacket(query->id, query->query_type, query->domain);
    return !query->w_buff.empty();//if empty, means format error
}

}
