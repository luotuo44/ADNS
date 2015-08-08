//Filename: Logger.cpp
//Date: 2015-6-2

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"Logger.hpp"
#include<string.h>
#include<string>
#include<time.h>
#include<stdio.h>

#include<sys/time.h>
#include<pthread.h> //for localtime_r



#include"LogBuffer.hpp"
#include"CurrentThread.hpp"


namespace ADNS
{

static void defaultOutput(const char* msg, int len)
{
    fwrite(msg, 1, len, stdout);
}

static void defaultFlush()
{
    fflush(stdout);
}


Log::OutputFunc Log::m_log_output = defaultOutput;
Log::FlushFunc Log::m_log_flush = defaultFlush;
Log::LogLevel Log::m_log_level = Log::INFO;

void Log::setLogLevel(LogLevel level)
{
    m_log_level = level;
}



void Log::setOutput(OutputFunc out)
{
    if(out)
        m_log_output = out;
}


void Log::setFlush(FlushFunc flush)
{
    if(flush)
        m_log_flush = flush;
}


static const char * level_str[Log::LEVEL_NUM] =
{
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};



Log::Log(const char *file, const char *func, int line, LogLevel level)
    : m_curr_level(level)
{
    addStrTime();

    CurrentThread::tid();//make sure has has tid
    m_log_buf.append("T:", 2);
    m_log_buf.append(CurrentThread::tidString());

    m_log_buf<<'[';
    m_log_buf.append(level_str[level]);
    m_log_buf<<']'<<' ';


    m_log_buf.append(file);
    m_log_buf<<':';
    m_log_buf.append(func);
    m_log_buf<<':';
	m_log_buf<<line<<' ';
}


Log::~Log()
{
    m_log_buf<<'\n';
    m_log_output(m_log_buf.data(), m_log_buf.size());

    if( m_curr_level == Log::FATAL)
    {
        m_log_flush();
        abort();
    }
}



void Log::addStrTime()
{
    struct timeval now;
    ::gettimeofday(&now, NULL);
    time_t second = now.tv_sec;
    int micro_second = now.tv_usec / 1000;


    struct tm tm_time;
    localtime_r(&second, &tm_time);


    char str_time[30];
    snprintf(str_time, sizeof(str_time), "%4d%02d%02d %02d:%02d:%02d.%06d ",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, micro_second);


    m_log_buf.append(str_time);
}

}
