//Filename: CurrentThread.hpp
//Date: 2015-5-27

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef CURRENTTHREAD_HPP
#define CURRENTTHREAD_HPP


namespace ADNS
{

namespace CurrentThread
{
    extern __thread int thread_tid;
    extern __thread char tid_string[15];
    extern __thread const char* thread_name;


    void gettid();

    inline int tid()
    {
        if( thread_tid == 0 )
            gettid();

        return thread_tid;
    }

    inline const char* tidString()
    {
        return tid_string;
    }

    inline const char* threadName()
    {
        return thread_name;
    }
}

}

#endif // CURRENTTHREAD_HPP
