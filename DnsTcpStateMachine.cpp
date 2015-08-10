//Filename: DnsTcpStateMachine.cpp
//Date: 2015-7-27

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"DnsTcpStateMachine.hpp"

#include<assert.h>
#include<string.h>

#include<vector>
#include<iostream>


#include"Reactor.hpp"
#include"SocketOps.hpp"
#include"Logger.hpp"
#include"DNS.hpp"
#include"typedef.hpp"

namespace ADNS
{

enum class DnsTcpState
{
    init,
    connecting_dns, //conn init state
    new_try, //one round start state
    send_query,
    recv_query_result,
    parse_query_result,
    stop_query_dns
};


struct DnsTcpStateMachine::QueryPacket
{
    int id;
    int fd;
    DnsTcpState state;
    Net::EventPtr ev;
    std::string domain;
    std::string dns_server;//ip
    DNSQueryType query_type;
    DnsQueryCB cb;
    uint32_t ttl;

    RecordData ips;//dns query result

    std::vector<char> r_buff;
    int r_curr;
    std::vector<char> w_buff;
    int w_curr;

    std::string fail_reason;
};


DnsTcpStateMachine::DnsTcpStateMachine(EventCreater &ev_creater)
    : m_ev_creater(ev_creater),
      m_res_cb(nullptr)
{
    assert(ev_creater);
}

DnsTcpStateMachine::~DnsTcpStateMachine()
{

}


void DnsTcpStateMachine::setResCB(DnsExplorerResCB &cb)
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

void DnsTcpStateMachine::addQuery(const DnsQuery_t &query)
{
    QueryPacketPtr q = std::make_shared<QueryPacket>();
    q->id = query.id;
    q->domain = query.domain;
    q->dns_server = query.dns_server;

    q->query_type = query.query_type;
    q->cb = query.cb;

    int ret = Net::SocketOps::new_tcp_socket_connect_server(query.dns_server.c_str(), 53, &q->fd);
    if( ret == -1 )
    {
        q->fail_reason = getError("new_tcp_socket_connect_server fail ");
        LOG(Log::ERROR)<<q->fail_reason<<" dns server : "<<q->dns_server<<" for query "<<q->domain;
        replyResult(q, false);
        return ;
    }

    updateEvent(q, EV_WRITE|EV_PERSIST);
    if( ret == 0)//connect success
        q->state = DnsTcpState::new_try;
    else
        q->state = DnsTcpState::connecting_dns;

    Net::Reactor::addEvent(q->ev);
    m_querys[q->fd] = q;
}


void DnsTcpStateMachine::replyResult(QueryPacketPtr &q, bool success)
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


void DnsTcpStateMachine::updateEvent(QueryPacketPtr &query, int new_events, int milliseconds)
{
    Net::Reactor::delEvent(query->ev);
    query->ev = m_ev_creater(query->fd, new_events, std::bind(&DnsTcpStateMachine::eventCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), nullptr);
    Net::Reactor::addEvent(query->ev, milliseconds);
}


void DnsTcpStateMachine::eventCB(int fd, int events, void *arg)
{
    auto it = m_querys.find(fd);
    if( it == m_querys.end() )
    {
        std::cout<<"event update a fd that does exist"<<std::endl;
        return ;
    }

    if( events & EV_TIMEOUT )
    {
        it->second->fail_reason = "timeout to wait for dns_server ";
        LOG(Log::ERROR)<<it->second->fail_reason<<it->second->dns_server<<" for query "<<it->second->domain;
        Net::Reactor::delEvent(it->second->ev);
        Net::SocketOps::close_socket(it->second->fd);
        replyResult(it->second, false);
        m_querys.erase(it);
        return ;
    }

    int query_state = driveMachine(it->second);
    if( query_state != 0 )
    {
        replyResult(it->second, query_state == 1);
        Net::Reactor::delEvent(it->second->ev);
        Net::SocketOps::close_socket(it->second->fd);
        m_querys.erase(it);
    }

    (void)events;(void)arg;
}



//return -1, means some error happen in this query.
//return 0, this state machine still needs to run
//return 1, means that it has finished query
int DnsTcpStateMachine::driveMachine(QueryPacketPtr &q)
{
    bool stop = false;
    int ret;
    uint16_t left;
    int query_state = 0;

    while( !stop )
    {
        switch(q->state)
        {
        case DnsTcpState::connecting_dns:
            ret = Net::SocketOps::connecting_server(q->fd);
            if( ret == 1 )//need to wait
                stop = true;
            else if(ret == -1 )//some error to this socket
            {
                q->fail_reason = getError("fail to connect dns server");
                LOG(Log::ERROR)<<q->fail_reason<<q->dns_server<<" for query "<<q->domain;
                query_state = -1;
                q->state = DnsTcpState::stop_query_dns;
            }
            else if(ret == 0 )//connect success
            {
                q->state = DnsTcpState::new_try;
            }

            break;

        case DnsTcpState::new_try:
            updateEvent(q, EV_WRITE|EV_PERSIST);
            if( getDNSQueryPacket(q) )
                q->state = DnsTcpState::send_query;
            else
            {
                q->fail_reason = "hostname format error ";
                LOG(Log::ERROR)<<q->domain<<' '<<q->fail_reason;
                query_state = -1;
                q->state = DnsTcpState::stop_query_dns;
            }
            break;

        case DnsTcpState::send_query:
            left = q->w_buff.size() - q->w_curr;
            ret = Net::SocketOps::write(q->fd, reinterpret_cast<char*>(&q->w_buff[q->w_curr]), left);

            if( ret < 0 )//socket error
            {
                q->fail_reason = getError("fail to send query to dns_server ");
                LOG(Log::ERROR)<<q->fail_reason<<q->dns_server<<"for query "<<q->domain;
                query_state = -1;
                q->state = DnsTcpState::stop_query_dns;
                break;
            }

            //else if( ret == 0)//send 0 bytes
            //   stop = true;

            q->w_curr += ret;
            if( ret == left)//has send all data
            {
                q->state = DnsTcpState::recv_query_result;
                updateEvent(q, EV_READ|EV_PERSIST|EV_TIMEOUT, 60000);//60 seconds

                //init
                q->r_buff.resize(2);
                q->r_curr = 0;
            }
            stop = true;

            break;

        case DnsTcpState::recv_query_result:
            //std::cout<<"recv_query_result"<<std::endl;
            left = q->r_buff.size() - q->r_curr;
            ret = Net::SocketOps::read(q->fd, reinterpret_cast<char*>(&q->r_buff[q->r_curr]), left);

            if( ret < 0 )//socket error
            {   q->fail_reason = getError("fail to recv query result from dns_server ");
                LOG(Log::ERROR)<<q->fail_reason<<q->dns_server<<" for query "<<q->domain;
                query_state = -1;
                q->state = DnsTcpState::stop_query_dns;
                break;
            }
            else if( ret == 0)
                stop = true;

            q->r_curr += ret;
            if( ret != left )
                stop = true;
            else //has read all data
            {
                if(q->r_buff.size() == 2)//read the length information
                {
                    ::memcpy(&left, &q->r_buff[0], 2);
                    left = Net::SocketOps::ntohs(left);
                    q->r_buff.resize(2+left);
                    q->r_curr = 2;
                }
                else //read dns result information
                {
                    q->state = DnsTcpState::parse_query_result;
                }
            }

            break;

        case DnsTcpState::parse_query_result:
            q->ips = DNS::parseDNSResultPacket(&q->r_buff[2], q->r_buff.size()-2, q->ttl);
            if( q->ips.empty() )//error result
            {
                q->fail_reason = "fail to parse dns server response packet";
                LOG(Log::ERROR)<<q->fail_reason<<" for query "<<q->domain;
                query_state = -1;
            }
            else
            {
                q->fail_reason = "success";
                query_state = 1;//success
            }

            q->state = DnsTcpState::stop_query_dns;
            break;


        case DnsTcpState::stop_query_dns:
            stop = true;
            break;

        default:
            std::cout<<"unexpect case"<<std::endl;
            break;
        }
    }

    return query_state;
}


bool DnsTcpStateMachine::getDNSQueryPacket(QueryPacketPtr &query)
{
    std::string query_packet = DNS::getDNSPacket(query->id, query->query_type, query->domain);
    if( query_packet.empty() )//format error
        return false;

    query->w_buff.resize(query_packet.size() + 2);

    uint16_t t = Net::SocketOps::htons(query_packet.size());
    ::memcpy(&query->w_buff[0], &t, 2);
    ::memcpy(&query->w_buff[2], query_packet.c_str(), query_packet.size());
    query->w_curr = 0;

    return true;
}


}
