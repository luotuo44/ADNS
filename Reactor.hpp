//Filename: Reactor.hpp
//Date: 2015/6/6

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef REACTOR_HPP
#define REACTOR_HPP


#include<memory>
#include<functional>
#include<atomic>


namespace ADNS
{


namespace Net
{


#define  EV_READ 1
#define  EV_WRITE 2
#define  EV_TIMEOUT 4
#define  EV_PERSIST 8

typedef std::function<void (int fd, int events, void *arg)> EVENT_CB;


class Event;
typedef std::shared_ptr<Event> EventPtr;

class Poller;
typedef std::unique_ptr<Poller> PollerUPtr;
class Epoller;
typedef std::unique_ptr<Epoller> EpollerUPtr;

class Reactor;
typedef std::shared_ptr<Reactor> ReactorPtr;
typedef std::weak_ptr<Reactor> ReactorWPtr;

class Reactor
{
public:
    ~Reactor();

    int dispatch();
    void update(int fd, int events);

    EventPtr createEvent(int fd, int events, EVENT_CB cb, void *arg);

public:
    static ReactorPtr newReactor();
    static bool addEvent(EventPtr &ev, int millisecond = 0);
    static bool delEvent(EventPtr &ev);

private:
    Reactor();

    Reactor(const Reactor &)=delete;
    Reactor& operator = (const Reactor &)=delete;

private:
    class Helper;
    class Impl;
    class AsyncEvent;
    typedef std::unique_ptr<Helper> HelperUPtr;
    typedef std::unique_ptr<Impl> ImplUPtr;
    typedef std::unique_ptr<AsyncEvent> AsyncEventUPtr;

private:
    EpollerUPtr m_poller;
    HelperUPtr m_helper;
    ImplUPtr m_impl;
    AsyncEventUPtr m_async_events;//events that add by other thread

    std::atomic<bool> m_is_running;
    int m_running_tid;//id of the thread that call dispatch for this Reactor
    ReactorWPtr m_itself;
};

}

}

#endif // REACTOR_HPP
