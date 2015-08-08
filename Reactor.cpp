//Filename: Reactor.cpp
//Date: 2015/6/6

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#include"Reactor.hpp"

#include<unistd.h>

#include<assert.h>
#include<string.h>

#include<vector>
#include<map>
#include<list>
#include<algorithm>
#include<chrono>
#include<system_error>
#include<exception>

#include"Logger.hpp"
#include"Epoller.hpp"
#include"SocketOps.hpp"
#include"CurrentThread.hpp"
#include"MutexLock.hpp"

namespace ADNS
{


namespace Net
{

//this class is used to count the how many read and write events register in a fd
class Reactor::Helper
{
public:
    Helper()
    {
        m_count.reserve(32);
    }

    ~Helper()=default;

    Helper(const Helper &)=delete;
    Helper& operator = (const Helper &)=delete;
    Helper(Helper &&)noexcept=default;
    Helper& operator = (Helper &&)noexcept=default;

    int addEvent(int fd, int events)
    {
        int ret = 0;
        if( (events & EV_READ)  && addReadEvent(fd) )
            ret |= EV_READ;//first register read event for fd

        if( (events & EV_WRITE) && addWriteEvent(fd) )
            ret |= EV_WRITE;//first regitster write event for fd

        return ret;
    }


    int delEvent(int fd, int events)
    {
        int ret = 0;
        if( (events & EV_READ)  && delReadEvent(fd) )
            ret |= EV_READ;//first event deregister read event for fd

        if( (events & EV_WRITE) && delWriteEvent(fd) )
            ret |= EV_WRITE;//first event deregitster write event for fd

        return ret;
    }

private:
    bool isValid(int fd)
    {
        return (fd>=0) && (static_cast<size_t>(fd)<m_count.size());
    }

    //if it is the first read/write event register for a fd,
    //return true, or return false;
    bool addReadEvent(int fd);
    bool addWriteEvent(int fd);

    //if it is the last read/write event deregister for a fd,
    //return true, or return false;
    bool delReadEvent(int fd)
    {
        if( !isValid(fd) && (m_count[fd].r_num<=0) )
            return false;

        return --m_count[fd].r_num == 0;
    }

    bool delWriteEvent(int fd)
    {
        if( !isValid(fd) && (m_count[fd].w_num<=0) )
            return false;

        return --m_count[fd].w_num == 0;
    }

private:
    typedef struct helper_tag
    {
        helper_tag(helper_tag &&)noexcept=default;
        helper_tag& operator = (helper_tag &&)noexcept=default;

        int r_num;
        int w_num;

        helper_tag()
            : r_num(0), w_num(0)
        {}
    }helper_t;

private:
    std::vector<helper_t> m_count;
};


bool Reactor::Helper::addReadEvent(int fd)
{
    if(fd < 0)
        return false;

    if( static_cast<size_t>(fd) >= m_count.size() )
        m_count.resize(fd+1);

    return ++m_count[fd].r_num == 1;
}


bool Reactor::Helper::addWriteEvent(int fd)
{
    if(fd < 0)
        return false;

    if( static_cast<size_t>(fd) >= m_count.size() )
        m_count.resize(fd+1);

    return ++m_count[fd].w_num == 1;
}


//-------------------------------------------------------------------

enum class EV_STATE
{
    S_FREE,
    S_LINK
};

class Event
{
public:
    Event(int fd, int events, EVENT_CB cb, void *arg)
        : m_fd(fd), m_events(events), m_re_events(0),
          m_cb(cb), m_arg(arg),
          m_state(EV_STATE::S_FREE), m_is_internal(false)
    {}

public:
    int m_fd;
    int m_events;
    int m_re_events;
    EVENT_CB m_cb;
    void *m_arg;
    int m_internal;//Millisecond
    EV_STATE m_state;
    bool m_is_internal;//is the internal for reactor

    std::chrono::milliseconds m_duration;
    std::chrono::steady_clock::time_point m_active_time;
    ReactorWPtr reactor;
};


inline bool timerEventCmp(const EventPtr &lhs, const EventPtr &rhs)
{
    return lhs->m_active_time > rhs->m_active_time;
}



//--------------------------------

class Reactor::Impl
{
public:
    Impl() : m_event_num(0)
    {}

    ~Impl()=default;

    Impl(const Impl &)=delete;
    Impl& operator = (const Impl &)=delete;


    int eventNum()const { return m_event_num; }
    void update(int fd, int events);

    bool addEvent(EventPtr &ev);
    bool delEvent(EventPtr &ev);

    void handleActiveEvent();
    int minTimerDuration()const;//milliseconds

private:
    void addTimerEvent(EventPtr &ev);
    void delTimerEvent(EventPtr &ev);

private:
    std::multimap<int, EventPtr> m_events;
    std::multimap<int, EventPtr> m_active_events;

    std::vector<EventPtr> m_timer_events;
    int m_event_num;

private:
    auto findEvent(EventPtr &ev) -> decltype(m_events.end());
};


void Reactor::Impl::update(int fd, int events)
{
    if( events & EV_TIMEOUT )
    {
        assert(!m_timer_events.empty());

        m_timer_events[0]->m_re_events = m_timer_events[0]->m_events & EV_TIMEOUT;
        m_active_events.insert({m_timer_events[0]->m_fd, m_timer_events[0]});
        return ;
    }

    auto range = m_events.equal_range(fd);
    auto iter = range.first;
    while( iter != range.second )
    {
        if(iter->second->m_events & events)
        {
            iter->second->m_re_events = iter->second->m_events & events;
            m_active_events.insert(*iter);
        }

        ++iter;
    }
}


void Reactor::Impl::handleActiveEvent()
{
    auto it = m_active_events.begin();

    while(it != m_active_events.end())
    {
        //this EventPtr has beed deleted by former active event
        if( !(it->second->m_re_events & EV_TIMEOUT) && findEvent(it->second) == m_events.end())
        {
            m_active_events.erase(it++);
            continue;
        }

        if( !(it->second->m_events & EV_PERSIST) )
            Reactor::delEvent(it->second);
        else if( it->second->m_re_events & EV_TIMEOUT)//timer event active, and it was persist
        {
            delTimerEvent(it->second);
            addTimerEvent(it->second);
        }

        it->second->m_cb(it->second->m_fd, it->second->m_re_events, it->second->m_arg);
        m_active_events.erase(it++);
    }
}



bool Reactor::Impl::addEvent(EventPtr &ev)
{
    bool ret = true;
    do
    {
        if( (ev->m_events & EV_TIMEOUT ) && (ev->m_duration.count()>0) )
            addTimerEvent(ev);
        if( !(ev->m_events & (~(EV_TIMEOUT|EV_PERSIST))))//just timer event
            break;

        assert(ev->m_fd >= 0);

        if( findEvent(ev) == m_events.end() )
            m_events.insert({ev->m_fd, ev});
        else
        {
            LOG(Log::WARN)<<"add a event that has been added before";
            ret = false;
        }
    }while(0);

    m_event_num += ret ? 1 : 0;
    return ret;
}

void Reactor::Impl::addTimerEvent(EventPtr &ev)
{
    auto it = std::find(m_timer_events.begin(), m_timer_events.end(), ev);
    if( it != m_timer_events.end())
    {
        LOG(Log::WARN)<<"add a timer event that has beed added";
        return ;
    }

    ev->m_active_time = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(ev->m_duration);
    m_timer_events.push_back(ev);
    std::push_heap(m_timer_events.begin(), m_timer_events.end(), timerEventCmp);
}


int Reactor::Impl::minTimerDuration()const//milliseconds
{
    if( m_timer_events.empty() )
        return -1;

    EventPtr ev = m_timer_events.front();
    return std::chrono::duration_cast<std::chrono::milliseconds>(ev->m_active_time - std::chrono::steady_clock::now()).count();
}


bool Reactor::Impl::delEvent(EventPtr &ev)
{
    bool ret = true;
    do
    {
        if( ev->m_events & EV_TIMEOUT )
            delTimerEvent(ev);
        if( !(ev->m_events & (~(EV_TIMEOUT|EV_PERSIST))))//just timer event
            break;

        auto it = findEvent(ev);
        if( it != m_events.end() )
            m_events.erase(it);
        else
        {
            LOG(Log::WARN)<<"del a event that didn't exist";
            ret = false;
        }
    }while(0);

    m_event_num -= ret ? 1 : 0;
    return ret;
}


void Reactor::Impl::delTimerEvent(EventPtr &ev)
{
    auto it = std::find(m_timer_events.begin(), m_timer_events.end(), ev);
    if( it == m_timer_events.end() )
    {
        LOG(Log::ERROR)<<"del a timer event that didn't exist";
        return ;
    }

    if( it == m_timer_events.begin() )
    {
        std::pop_heap(m_timer_events.begin(), m_timer_events.end(), timerEventCmp);
        m_timer_events.pop_back();
    }
    else
    {
        m_timer_events.erase(it);
        std::make_heap(m_timer_events.begin(), m_timer_events.end(), timerEventCmp);
    }
}


auto Reactor::Impl::findEvent(EventPtr &ev)-> decltype(m_events.end())
{
    auto ret = m_events.end();

    do
    {
        auto range = m_events.equal_range(ev->m_fd);
        if( range.first == range.second )
            break;

        auto it = std::find_if(range.first, range.second, [&ev](decltype(*range.first) p)->bool{
                            return ev == p.second; //find it
                        });
        if( it != range.second)//find it
            ret = it;
    }while(0);

    return ret;
}




//#################################################

class NotifyEventList
{
public:
    std::list<EventPtr> ev_list;
    Mutex mutex;
};

class Reactor::AsyncEvent
{
public:
    AsyncEvent()=default;
    ~AsyncEvent()=default;
    AsyncEvent(const AsyncEvent&)=delete;
    AsyncEvent& operator = (const AsyncEvent&)=delete;


    void addOtherThreadEvent(EventPtr &ev);
    void delOtherThreadEvent(EventPtr &ev);

    void createNotifyEvent(ReactorPtr &reactor);
    void handleNotifyEvent(int fd, int events, void *arg);

    bool hasSomeEvent()const { return m_add_list.ev_list.size() + m_add_list.ev_list.size(); }
private:
    EventPtr m_notify_ev;
    int m_notify_pipe[2];

    NotifyEventList m_add_list;
    NotifyEventList m_del_list;
};


void Reactor::AsyncEvent::createNotifyEvent(ReactorPtr &reactor)
{
    assert(reactor);

    if( !SocketOps::new_pipe(m_notify_pipe, true, false) )
    {
        throw std::system_error(errno, std::system_category(), "Reactor::AsyncEvent::createNotifyEvent fail ");
    }


    m_notify_ev = reactor->createEvent(m_notify_pipe[0], EV_READ | EV_PERSIST,
                                       std::bind(&Reactor::AsyncEvent::handleNotifyEvent, this, m_notify_pipe[0], EV_READ, nullptr), nullptr);

    if( !m_notify_ev )
        throw std::runtime_error("fail to create notify event");

    m_notify_ev->m_is_internal = true;


    if( !Reactor::addEvent(m_notify_ev) )
        throw std::runtime_error("fail to add notify event");
}


void Reactor::AsyncEvent::addOtherThreadEvent(EventPtr &ev)
{
    assert(ev);
    MutexLock lock(m_add_list.mutex);
    m_add_list.ev_list.push_back(ev);

    SocketOps::writen(m_notify_pipe[1], "a", 1);
}


void Reactor::AsyncEvent::delOtherThreadEvent(EventPtr &ev)
{
    assert(ev);
    MutexLock lock(m_del_list.mutex);
    m_del_list.ev_list.push_back(ev);

    SocketOps::writen(m_notify_pipe[1], "d", 1);
}


void Reactor::AsyncEvent::handleNotifyEvent(int fd, int events, void *arg)
{
    char buf[1];
    while( 1 )
    {
        int ret = SocketOps::read(m_notify_pipe[0], buf, 1);
        if(ret == 0)//don't have any data to read
            break;
        //FIXME: should check ret == -1?

        switch(buf[0])
        {
        case 'a' :
        {
            MutexLock lock(m_add_list.mutex);
            EventPtr ev = std::move(m_add_list.ev_list.front());
            m_add_list.ev_list.pop_front();
            Reactor::addEvent(ev);
            break;
        }

        case 'd' :
        {
            MutexLock lock(m_del_list.mutex);
            EventPtr ev = std::move(m_del_list.ev_list.front());
            m_del_list.ev_list.pop_front();
            Reactor::delEvent(ev);
            break;
        }

        }//switch
    }

    (void)fd;(void)events;(void)arg;
}

//-----------------------------------------------

Reactor::Reactor()
    : m_poller(new Epoller),
      m_helper(new Helper),
      m_impl(new Impl),
      m_async_events(new AsyncEvent),
      m_is_running(false)
{
}


Reactor::~Reactor()
{
}

int Reactor::dispatch()
{
    m_running_tid = CurrentThread::tid();
    m_is_running.store(true);

    int ret = 0;
    while(1)
    {
        int event_num = m_impl->eventNum();
        if( event_num < 2 && !m_async_events->hasSomeEvent() )//implicit a notify event
        {
            ret = 0;
            break;
        }

        int time_duration = m_impl->minTimerDuration();
        ret = m_poller->dispatch(time_duration);
        if( ret <= 0)
            break;

        m_impl->handleActiveEvent();
    }

    if( ret == -1)
    {
        throw std::system_error(errno, std::system_category(), "epoll fail");
    }
    else if( ret == 0)
    {
        LOG(Log::INFO)<<"no event need to listen";
    }

	return ret;
}


void Reactor::update(int fd, int events)
{
    m_impl->update(fd, events);
}


EventPtr Reactor::createEvent(int fd, int events, EVENT_CB cb, void *arg)
{
    //fd < 0, but not just timer event
    if( fd < 0  && (events & (~(EV_TIMEOUT|EV_PERSIST))) )
        return EventPtr();

    EventPtr ev = std::make_shared<Event>(fd, events, cb, arg);
    ev->reactor = m_itself;

    return ev;
}



////////////static member function///////

///
/// \brief Reactor::newReactor
/// \return a new Reactor instance
///
ReactorPtr Reactor::newReactor()
{
    //ReactorPtr reactor = std::make_shared<Reactor>();
    ReactorPtr reactor(new Reactor);

    reactor->m_itself = reactor;
    reactor->m_async_events->createNotifyEvent(reactor);
    reactor->m_poller->setObserver(reactor);

    return reactor;
}

bool Reactor::addEvent(EventPtr &ev, int millisecond)
{
    bool ret = true;
    do
    {
        if( !ev || ev->m_events == 0 || ev->m_cb == nullptr || ev->m_state != EV_STATE::S_FREE)
        {
            ret = false;
            break;
        }

        ReactorPtr reactor = ev->reactor.lock();
        if( !reactor )
        {
            ret = false;
            break;
        }

        if( (ev->m_events & EV_TIMEOUT) && millisecond > 0)
            ev->m_duration = std::chrono::milliseconds(millisecond);

        //internal event for reactor, those event need to add then reactor is created
        if( ev->m_is_internal )
        {
            //pass
        }
        else
        {
            //when the reactor doesn't calls dispatch, m_is_running equals false, at this moment,
            //all event need to add into m_async_events as they are added by other thread.
            //current thread add a event to the Reactor that was run by
            //other thread. for thread safe, this ev will be added delay
            if( !reactor->m_is_running.load(std::memory_order_acquire) || reactor->m_running_tid != CurrentThread::tid())
            {
                reactor->m_async_events->addOtherThreadEvent(ev);
                ret = true;
                break;
            }
        }

        if( !reactor->m_impl->addEvent(ev)) 
        {
            ret = false;
            break;
        }

        //events stores those event that were first time add to fd
        int events = reactor->m_helper->addEvent(ev->m_fd, ev->m_events);
        if( events )//should register those event for the fd in OS API level
            reactor->m_poller->registerEvent(ev->m_fd, events);

        ev->m_state = EV_STATE::S_LINK;

        ret = true;
    }while(0);

    return ret;
}


bool Reactor::delEvent(EventPtr &ev)
{
    bool ret = true;
    do
    {
        if( !ev || ev->m_state == EV_STATE::S_FREE)
        {
            ret = false;
            break;
        }

        ReactorPtr reactor = ev->reactor.lock();
        if( !reactor )
        {
            ret = false;
            break;
        }

        //current thread del a event from the Reactor that created by
        //other thread. for thread safe, this ev will be del delay
        if( !reactor->m_is_running.load(std::memory_order_acquire) || reactor->m_running_tid != CurrentThread::tid() )
        {
            reactor->m_async_events->delOtherThreadEvent(ev);
            ret = true;
            break;
        }

        if( !reactor->m_impl->delEvent(ev) )
        {
            ret = false;
            break;
        }

        //events stores those event that weren't for fd
        int events = reactor->m_helper->delEvent(ev->m_fd, ev->m_events);
        if( events )//should unregister those event for the fd int OS API level
            reactor->m_poller->unRegisterEvent(ev->m_fd, events);

        ev->m_state = EV_STATE::S_FREE;

        ret = true;
    }while(0);

    return ret;
}


}

}
