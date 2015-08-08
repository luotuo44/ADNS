//Filename: Logger.hpp
//Date: 2015-6-2

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include"LogBuffer.hpp"

namespace ADNS
{

class Log
{
public:
	enum LogLevel
	{
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL,
		LEVEL_NUM
	};

    //for change the output function.
    //the default output function was write to stdout
    typedef void (*OutputFunc)(const char* msg, int len);
    typedef void (*FlushFunc) ();

    static void setOutput(OutputFunc func);
    static void setFlush(FlushFunc func);
    static void setLogLevel(LogLevel level);
    static LogLevel logLevel() { return m_log_level; }


public:
    Log(const char* file, const char* func, int line, LogLevel level);

    ~Log();

    Log(const Log &)=delete;
    Log& operator = (const Log& )=delete;

    LogBuffer& stream() { return m_log_buf; }

private:
    void addStrTime();

private:
    LogBuffer m_log_buf;
    LogLevel m_curr_level;

    static OutputFunc m_log_output;
    static FlushFunc m_log_flush;
    static LogLevel m_log_level;
};


#define LOG(level) Log(__FILE__, __func__, __LINE__, level).stream()

}

#endif // LOGGER_HPP
