//Filename: Condition.hpp
//Date: 2015-5-25

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef CONDITION_HPP
#define CONDITION_HPP


#include<pthread.h>
#include"MutexLock.hpp"


namespace ADNS
{

class Mutex;
class Condition
{
public:
    explicit Condition(Mutex &mutex);
    ~Condition();


    Condition(const Condition &)=delete;
    Condition& operator = (const Condition &)=delete;

    void wait();
    //if timeout return false. or return true
    bool waitForMilliseconds(int msecs);


    void notify();
    void notifyAll();


private:
    Mutex &m_mutex;
    pthread_cond_t m_cond;
};

}

#endif // CONDITION_HPP

