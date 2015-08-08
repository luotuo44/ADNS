//Filename: Thread.hpp
//Date: 2015-5-27

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef THREAD_HPP
#define THREAD_HPP

#include<pthread.h>

#include<functional>
#include<memory>
#include<string>

#include"MutexLock.hpp"
#include"Condition.hpp"


namespace ADNS
{

class Thread
{
public:
    typedef std::function<void(void*)> ThreadFunc;

public:
    explicit Thread(ThreadFunc func, const std::string &thread_name = "");
    ~Thread();

    Thread(const Thread& )=delete;
    Thread& operator = (const Thread& )=delete;

    //this function isn't thread safe
    void start(void *arg);
    void join();

    bool isStart()const { return m_started; }
    pid_t tid()const { return m_tid; }
    const std::string& threadNname()const { return m_thread_name; }

private:
    static void* realStartThread(void* arg);

private:
    pthread_t m_thread_id;
    pid_t m_tid;
    std::string m_thread_name;

    bool m_started;
    bool m_really_run;
    ThreadFunc m_func;
    void *m_arg;

    Mutex m_mutex;
    Condition m_cond;
};

}

#endif // THREAD_HPP
