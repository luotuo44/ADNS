//Filename: Epoller.cpp
//Date: 2015-6-27

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#include"Epoller.hpp"


#include<assert.h>
#include<stdlib.h>
#include<system_error>

#include"Reactor.hpp"
#include"Logger.hpp"


namespace ADNS
{

namespace Net
{

Epoller::Epoller()
    : m_enable_et(true)
{
    m_efd = ::epoll_create(1);
    if( m_efd == -1)
        throw std::system_error(errno, std::system_category(), "epoll_create fail");

    m_wr_count.reserve(32);
}


void Epoller::setObserver(ReactorPtr &observer)
{
    assert(observer);
    m_observer = observer;
}


int Epoller::listenEvent(int fd, int events)
{
    assert(fd >= 0);

    int ev = 0;
    if( events & EV_READ)
    {
        m_wr_count[fd].setR();
        ev |= EPOLLIN;
    }
    if( events & EV_WRITE)
    {
        m_wr_count[fd].setW();
        ev |= EPOLLOUT;
    }

    return ev;
}


int Epoller::hasWhatEvent(int fd)
{
    assert(fd >= 0);

    int events = 0;
    if( m_wr_count[fd].hasW() )
        events |= EV_WRITE;
    if( m_wr_count[fd].hasR() )
        events |= EV_READ;

    return events;
}


void Epoller::registerEvent(int fd, int events)
{
    assert(fd >= 0);
    if( !judgeFd(fd) )
        return;

    int ctl = 0;
    if( m_wr_count[fd].isNone() )
        ctl = EPOLL_CTL_ADD;
    if( m_wr_count[fd].isHaving() )
    {
        ctl = EPOLL_CTL_MOD;
        events |= hasWhatEvent(fd);
    }

    int ev = listenEvent(fd, events);
    addOrModFd(fd, ev, ctl);
}


void Epoller::unRegisterEvent(int fd, int events)
{
    if( fd < 0 || (static_cast<size_t>(fd)) >= m_wr_count.size() || m_wr_count[fd].isNone())
        return ;

    int ev = hasWhatEvent(fd);
    ev &= ~events;//minus events
    if( ev == 0)//left nothing
    {
        m_wr_count[fd].reset();
        delFd(fd);
    }
    else
    {
        int mod_ev = listenEvent(fd, ev);
        addOrModFd(fd, mod_ev, EPOLL_CTL_MOD);
    }
}


bool Epoller::judgeFd(int fd)
{
    if( static_cast<size_t>(fd) >= m_wr_count.size() )
    {
        //should not use push_back
        m_wr_count.resize(fd+1);

        if( m_fd_vec.size() < static_cast<size_t>(fd+1) )
            m_fd_vec.resize(fd+1);
    }

    return true;
}



void Epoller::addOrModFd(int fd, int events, int ctl)
{
    assert( fd >= 0);
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;

    if( m_enable_et)
        ev.events |= EPOLLET;

    if( ::epoll_ctl(m_efd, ctl, fd, &ev) == -1)
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        LOG(Log::ERROR)<<"epoll_ctl fail "<<buf;
    }
}


void Epoller::delFd(int fd)
{
    assert( fd >= 0);
    if( ::epoll_ctl(m_efd, EPOLL_CTL_DEL, fd, nullptr) == -1)
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        LOG(Log::ERROR)<<"epoll_ctl fail "<<buf;
    }
}


int Epoller::dispatch(int millisecond)
{
    int ret;
    while(1)
    {
        ret = ::epoll_wait(m_efd, &m_fd_vec[0], m_fd_vec.size(), millisecond);
        if (ret == -1 && errno == EINTR)
            continue;
        else
            break;
    }

    ReactorPtr reactor = m_observer.lock();
    if( !reactor )
    {
        LOG(Log::ERROR)<<"observer is empty";
        return -1;
    }

    if( ret == 0)//timeout event
    {
        reactor->update(-1, EV_TIMEOUT);
        return 1;
    }

    for(int i = 0; i < ret; ++i)
    {
        int what = m_fd_vec[i].events;
        int events = 0;
        if( what & EPOLLIN )
            events |= EV_READ;
        if( what & EPOLLOUT )
            events |= EV_WRITE;
        if( what & (EPOLLERR|EPOLLHUP) )
            events |= EV_READ | EV_WRITE;

        reactor->update(m_fd_vec[i].data.fd, events);
    }

    return ret;
}


}

}
