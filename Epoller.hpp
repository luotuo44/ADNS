//Filename: Epoller.hpp
//Date: 2015-6-27

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef EPOLLER_HPP
#define EPOLLER_HPP

#include<sys/epoll.h>

#include<vector>
#include<memory>

namespace ADNS
{

namespace Net
{

class Reactor;
typedef std::weak_ptr<Reactor> ReactorWPtr;
typedef std::shared_ptr<Reactor> ReactorPtr;

class Epoller
{
public:
    Epoller();
    ~Epoller()=default;
    Epoller(const Epoller& )=delete;
    Epoller& operator = (const Epoller &)=delete;
    Epoller(Epoller &&p)=default;
    Epoller& operator = (Epoller &&p)=default;

    void setObserver(ReactorPtr &observer);

    void registerEvent(int fd ,int events);
    void unRegisterEvent(int fd, int events);

    int dispatch(int millisecond);

private:
    int listenEvent(int fd, int events);
    int hasWhatEvent(int fd);
    bool judgeFd(int fd);
    void addOrModFd(int fd,int events, int ctl);
    void delFd(int fd);

    typedef struct wr_tag
    {
        bool w;
        bool r;

        wr_tag()
            : w(false), r(false)
        {}

        bool isNone()const { return !(w|r); }
        bool isHaving()const { return w|r; }
        bool isBoth()const { return w&r;}
        bool hasW()const { return w;}
        bool hasR()const { return r; }
        void reset() { w = r = false; }
        void setW() { w = true; }
        void setR() { r = true; }
    }wr_t;

private:
    int m_efd;
    bool m_enable_et;

    std::vector<wr_t> m_wr_count;
    std::vector<epoll_event> m_fd_vec;
    ReactorWPtr m_observer;//destroyer shouldn't delete it
};


}

}

#endif // EPOLLER_HPP
