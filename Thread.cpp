//Filename: Thread.cpp
//Date: 2015-5-27

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"Thread.hpp"

#include<sys/types.h>
#include<sys/syscall.h>
#include<unistd.h>

#include<stdio.h>
#include<string.h>
#include<assert.h>
#include<stdlib.h>



#include"CurrentThread.hpp"
#include"Logger.hpp"

namespace ADNS
{

namespace CurrentThread {

    __thread int thread_tid = 0;
    __thread char tid_string[15];
    __thread const char* thread_name = "unkown";

    void gettid()
    {
        if( thread_tid == 0 )
        {
            thread_tid = static_cast<pid_t>(::syscall(SYS_gettid));
            snprintf(tid_string, sizeof(tid_string), "%5d ", thread_tid);
        }
    }
}



static void assertThread(const char *op, int result)
{
    if(result)
    {
        char buf[50];
        snprintf(buf, sizeof(buf), "%m");
        LOG(Log::FATAL)<<op<<" fail : "<<buf;
    }
}


Thread::Thread(ThreadFunc func, const std::string &thread_name)
    : m_thread_id(0),
      m_tid(0),
      m_thread_name(thread_name),
      m_started(false),
      m_really_run(false),
      m_func(func),
      m_arg(nullptr),
      m_mutex(),
      m_cond(m_mutex)
{

}



Thread::~Thread()
{
    {
        MutexLock lock(m_mutex);
        //if user call start() function, let the thread really runs
        //before destruct the Thread object.
        while( m_started && !m_really_run)
            m_cond.wait();
    }
}


//this function isn't thread safe
void Thread::start(void *arg)
{
    if(m_started)
    {
        LOG(Log::WARN)<<"start a thread more than one time";
        return ;
    }

    m_started = true;
    m_arg = arg;

    int ret = pthread_create(&m_thread_id, nullptr, Thread::realStartThread, this);

    assertThread("create thread", ret);
}


void Thread::join()
{
    if(!m_started)
    {
        LOG(Log::WARN)<<"join a Thread that doesn't start";
        return ;
    }

    pthread_join(m_thread_id, nullptr);//ignore the return value
}


void* Thread::realStartThread(void *arg)
{
    Thread* thread = reinterpret_cast<Thread*>(arg);
    assert(thread != nullptr);

    thread->m_tid = CurrentThread::tid();
    CurrentThread::thread_name = thread->m_thread_name.c_str();

    auto fun = thread->m_func;//save it
    auto tmp_arg = thread->m_arg;

    {
        MutexLock lock(thread->m_mutex);
        thread->m_really_run = true;
        thread->m_cond.notify();
    }



    if( fun != nullptr)
        fun(tmp_arg);

	return nullptr;
}

}
