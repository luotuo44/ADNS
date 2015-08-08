//Filename: MutexLock.cpp
//Date: 2015-5-25

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"MutexLock.hpp"
#include<system_error>



namespace ADNS
{



Mutex::Mutex()
{
    int ret = pthread_mutex_init(&m_mutex, nullptr);
    if( ret != 0 )
        throw std::system_error(ret, std::system_category(), "init mutex fail");
}


Mutex::~Mutex()
{
    int ret = pthread_mutex_destroy(&m_mutex);
    if( ret != 0 )
        throw std::system_error(ret, std::system_category(), "destroy mutex fail");
}


void Mutex::lock()
{
    int ret = pthread_mutex_lock(&m_mutex);
    if( ret != 0 )
        throw std::system_error(ret, std::system_category(), "lock mutex fail");
}


void Mutex::unlock()
{
    int ret = pthread_mutex_unlock(&m_mutex);

    if( ret != 0 )
        throw std::system_error(ret, std::system_category(), "unlock mutex fail");
}


}
