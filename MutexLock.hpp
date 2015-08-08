//Filename: MutexLock.hpp
//Date: 2015-5-25

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef MUTEXLOCK_HPP
#define MUTEXLOCK_HPP


#include<pthread.h>


namespace ADNS
{

class Condition;

class Mutex
{
public:
    Mutex();
    ~Mutex();

    Mutex(const Mutex &)=delete;
    Mutex& operator = (const Mutex &)=delete;

    void lock();
    void unlock();

    friend class Condition;

private:
    pthread_mutex_t m_mutex;
};



class MutexLock
{
public:
    explicit MutexLock(Mutex &mutex)
        : m_mutex(mutex)
    {
        m_mutex.lock();
    }

    ~MutexLock()
    {
        m_mutex.unlock();
    }

    MutexLock(const MutexLock &)=delete;
    MutexLock& operator = (const MutexLock &)=delete;

private:
    Mutex &m_mutex;
};



#define MutexLock(x) error "Missing lock a mutex"

}


#endif // MUTEXLOCK_HPP
