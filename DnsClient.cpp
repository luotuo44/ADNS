//Filename: DnsClient.cpp
//Date: 2015-7-16

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"DnsClient.hpp"

#include<assert.h>
#include<stdio.h>

#include<list>
#include<system_error>
#include<functional>
#include<atomic>
#include<iostream>

#include"SocketOps.hpp"
#include"DnsCache.hpp"
#include"MutexLock.hpp"
#include"Condition.hpp"
#include"Reactor.hpp"
#include"Thread.hpp"
#include"DnsExplorer.hpp"
#include"typedef-internal.hpp"

namespace ADNS
{

class DnsClient::Impl
{
public:
    Impl(DnsCacheQueryFun cache_query, DnsCacheSaveFun cache_save, DnsExplorerQueryFun explorer_query);
    ~Impl();

    Impl(const Impl &)=delete;
    Impl& operator = (const Impl&)=delete;

public:
    void init(Net::ReactorPtr &reactor);
    void addQuery(const DnsQuery_t &q);
    void addQuery(DnsQuery_t &&q);
    void addResult(DnsQueryResult_t &&result);

    void event_cb(int fd, int events, void *arg);

public:
    typedef std::function<void (const DnsQuery_t &q)> DnsCacheQueryCB;
    static void threadFun(void *arg);

private:
    void handleNewQuery();
    void handleNewResult();

private:
    std::list<DnsQuery_t> m_querys;
    Mutex m_query_mutex;

    std::list<DnsQueryResult_t> m_results;
    Mutex m_result_mutex;

    int m_pipe[2];
    Net::EventPtr m_ev;

    DnsCacheQueryFun m_cache_query_fun;
    DnsCacheSaveFun m_cache_save_fun;

    DnsExplorerQueryFun m_explorer_query_fun;
};


DnsClient::Impl::Impl(DnsCacheQueryFun cache_query, DnsCacheSaveFun cache_save, DnsExplorerQueryFun explorer_query)
   :  m_pipe{-1, -1},
      m_cache_query_fun(cache_query),
      m_cache_save_fun(cache_save),
      m_explorer_query_fun(explorer_query)
{
    assert(cache_query != nullptr);
    assert(cache_save != nullptr);
    assert(explorer_query != nullptr);
}



DnsClient::Impl::~Impl()
{
    //ignore return value
    Net::Reactor::delEvent(m_ev);
    Net::SocketOps::close_socket(m_pipe[0]);
    Net::SocketOps::close_socket(m_pipe[1]);
}


void DnsClient::Impl::init(Net::ReactorPtr &reactor)
{
    if( !Net::SocketOps::new_pipe(m_pipe, true, false) )
    {
        char buf[100];
        snprintf(buf, sizeof(buf), "DnsClient::Impl::Impl new_pipe fail : %m");
        throw std::system_error(errno, std::system_category(), buf);
    }

    assert(reactor);
    m_ev = reactor->createEvent(m_pipe[0], EV_READ|EV_PERSIST,
                            std::bind(&DnsClient::Impl::event_cb, this, std::placeholders::_1, std::placeholders::_1, nullptr), nullptr);

    Net::Reactor::addEvent(m_ev);
}

void DnsClient::Impl::addQuery(const DnsQuery_t &q)
{
    {
        MutexLock lock(m_query_mutex);
        m_querys.push_back(q);
    }

    Net::SocketOps::writen(m_pipe[1], "q", 1);
}


void DnsClient::Impl::addQuery(DnsQuery_t &&q)
{
    {
        MutexLock lock(m_query_mutex);
        m_querys.push_back(std::move(q));
    }

    Net::SocketOps::writen(m_pipe[1], "q", 1);
}


void DnsClient::Impl::addResult(DnsQueryResult_t &&result)
{
    {
        MutexLock lock(m_result_mutex);
        m_results.push_back(std::move(result));
    }

    Net::SocketOps::writen(m_pipe[1], "r", 1);
}



void DnsClient::Impl::event_cb(int fd, int events, void *arg)
{
    assert(fd == m_pipe[0]);
    assert(events = EV_READ);

    char buf[1];
    int ret;

    while(1)
    {
        ret = Net::SocketOps::read(fd, buf, 1);
        if(ret == 0)//don't have any data to read
            break;

        //FIXME: should check ret == -1?

        switch(buf[0])
        {
        case 'q' :
            handleNewQuery();
            break;

        case 'r' :
            handleNewResult();
            break;
        }
    }

    (void)fd;(void)events;(void)arg;
}


void DnsClient::Impl::handleNewQuery()
{
    DnsQuery_t q;
    {
        MutexLock lock(m_query_mutex);
        q = std::move(m_querys.front());
        m_querys.pop_front();
    }


    auto result = m_cache_query_fun(q);
    if( result.empty() )//doesn't exist result
        m_explorer_query_fun(std::move(q));
    else
    {
        DnsQueryResult_t res;
        res.id = q.id;
        res.success = true;
        res.fail_reason = "success";
        res.query_type = q.query_type;
        res.cb = q.cb;
        res.domain = std::move(q.domain);
        res.dns_server = "local DnsCache";
        res.record_data = std::move(result);

        q.cb(std::move(res));
    }
}


void DnsClient::Impl::handleNewResult()
{
    DnsQueryResult_t res;
    {
        MutexLock lock(m_result_mutex);
        res = std::move(m_results.front());
        m_results.pop_front();
    }

    if( res.success )
    {
        m_cache_save_fun(res);
    }
        
    assert(res.cb != nullptr);
    auto cb = res.cb;
    cb(std::move(res));
}



DnsClientPtr DnsClient::dns_client;
static Mutex g_init_mutex;
static Condition g_init_cond(g_init_mutex);
static Thread g_init_thread(DnsClient::Impl::threadFun);
static std::atomic<bool> g_finish_init(false);

void DnsClient::Impl::threadFun(void *arg)
{
    Net::ReactorPtr reactor = Net::Reactor::newReactor();
    EventCreater  ev_creater = std::bind(&Net::Reactor::createEvent, reactor, std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4);

    DnsCachePtr dns_cache = std::make_shared<DnsCache>();
    DnsExplorerPtr dns_explorer = std::make_shared<DnsExplorer>(ev_creater);
    dns_explorer->init();

    DnsCacheQueryFun cache_query_fun = std::bind(&DnsCache::query, dns_cache, std::placeholders::_1);
    DnsCacheSaveFun cache_save_fun = std::bind(&DnsCache::flush, dns_cache, std::placeholders::_1);
    DnsExplorerQueryFun explorer_query_fun = std::bind(&DnsExplorer::addDnsRequest, dns_explorer, std::placeholders::_1);

    dns_client = DnsClientPtr(new DnsClient);
    dns_client->m_impl = std::shared_ptr<Impl>(new Impl(cache_query_fun, cache_save_fun, explorer_query_fun) );

    dns_client->m_impl->init(reactor);
    dns_explorer->setDnsClientResCB(std::bind(&DnsClient::Impl::addResult, dns_client->m_impl, std::placeholders::_1));

    g_finish_init.store(true);
    g_init_cond.notify();

    reactor->dispatch();

    (void)arg;
}


//---------------------------------------------------------------


bool DnsClient::addQuery(const DnsQuery_t &query)
{
    if( !isLegalQuery(query) )
        return false;

    assert(query.cb != nullptr);
    m_impl->addQuery(query);

    return true;
}

bool DnsClient::addQuery(DnsQuery_t &&query)
{
    if( !isLegalQuery(query))
        return false;

    assert(query.cb != nullptr);
    m_impl->addQuery(std::move(query));

    return true;
}



bool DnsClient::isLegalQuery(const DnsQuery_t &query)
{
    return Net::SocketOps::isLegalIP(query.dns_server.c_str());
}


void DnsClient::init()
{
    g_init_thread.start(nullptr);

    MutexLock lock(g_init_mutex);
    while( !g_finish_init.load() )
        g_init_cond.wait();
}

DnsClientPtr DnsClient::getInstance()
{
    return dns_client;
}


}
