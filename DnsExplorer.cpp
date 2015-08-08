//Filename: DnsExplorer.cpp
//Date: 2015-7-16

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"DnsExplorer.hpp"

#include<assert.h>
#include<stdio.h>


#include<functional>
#include<system_error>

#include"Reactor.hpp"
#include"SocketOps.hpp"
#include"DnsTcpStateMachine.hpp"

namespace ADNS
{


DnsExplorer::DnsExplorer(EventCreater &ev_creater)
    : m_ev_creater(ev_creater),
      m_pipe{-1, -1},
      m_tcp_machine(std::make_shared<DnsTcpStateMachine>(ev_creater)),
      m_minUnusedID(2048)
{
    assert(ev_creater);
}


DnsExplorer::~DnsExplorer()
{
    Net::SocketOps::close_socket(m_pipe[0]);
    Net::SocketOps::close_socket(m_pipe[1]);
    Net::Reactor::delEvent(m_pipe_ev);
}


void DnsExplorer::init()
{
    if( !Net::SocketOps::new_pipe(m_pipe, true, false) )
    {
        char buf[100];
        snprintf(buf, sizeof(buf), "DnsExplorer::DnsExplorer new_pipe fail : %m");
        throw std::system_error(errno, std::system_category(), buf);
    }

    updateEvent(m_pipe[0], EV_READ|EV_PERSIST, -1);

    DnsExplorerResCB cb = std::bind(&DnsExplorer::DnsResultCB, this, std::placeholders::_1);
    m_tcp_machine->setResCB(cb);
    //m_udp_machine->setResCB(cb);
}


void DnsExplorer::setDnsClientResCB(DnsClientResCB cb)
{
    assert(cb != nullptr);
    m_client_res_cb = cb;
}


void DnsExplorer::updateEvent(int fd, int new_events, int milliseconds)
{
    Net::Reactor::delEvent(m_pipe_ev);
    m_pipe_ev = m_ev_creater(fd, new_events, std::bind(&DnsExplorer::pipe_cb, this, std::placeholders::_1, std::placeholders::_2, nullptr), nullptr);
    Net::Reactor::addEvent(m_pipe_ev, milliseconds);
}


void DnsExplorer::addDnsRequest(DnsQuery_t &&query)
{
    {
        MutexLock lock(m_request_mutex);
        m_requests.push_back(std::move(query));
    }

    Net::SocketOps::writen(m_pipe[1], "q", 1);
}


void DnsExplorer::restoreRequest(DnsQuery_t &&query)
{
    {
        MutexLock lock(m_request_mutex);
        m_requests.push_front(std::move(query));
    }

    Net::SocketOps::writen(m_pipe[1], "q", 1);
}

void DnsExplorer::pipe_cb(int fd, int events, void *arg)
{
    assert(fd == m_pipe[0]);
    assert(events == EV_READ);

    char buf[1];
    int ret;

    while(1)
    {
        ret = Net::SocketOps::read(fd, buf, 1);
        if(ret == 0)//don't have any data to read
            break;

        //FIXME: should check ret == -1?
        DnsQuery_t query;
        {
            MutexLock lock(m_request_mutex);
            query = std::move(m_requests.front());
            m_requests.pop_front();
        }

        if( !handleNewRequest(std::move(query)) )
            break;
    }

    (void)fd;(void)events;(void)arg;
}





bool DnsExplorer::handleNewRequest(DnsQuery_t &&query)
{
    int id = m_minUnusedID.generateID();
    if( id != -1 )
    {
        m_tcp_machine->addQuery(id, query.domain, query.dns_server, query.cb);
        return true;
    }
    else
    {
        restoreRequest(std::move(query));
        updateEvent(m_pipe[0], EV_TIMEOUT, 5000);//wait for 5 seconds
        return false;
    }
}


void DnsExplorer::DnsResultCB(DnsQueryResult_t &&res)
{
    m_minUnusedID.releaseID(res.id);
    m_client_res_cb(std::move(res));
}

}


