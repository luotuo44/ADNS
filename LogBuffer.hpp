//Filename: LogBuffer.hpp
//Date: 2015-6-2

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef LOGBUFFER_HPP
#define LOGBUFFER_HPP


#include<string>

namespace ADNS
{

class LogBuffer
{
public:
    LogBuffer();


    LogBuffer(const LogBuffer &)=delete;
    LogBuffer& operator = (const LogBuffer &)=delete;

public:
    LogBuffer& operator << (short v);
    LogBuffer& operator << (unsigned short v);
    LogBuffer& operator << (int v);
    LogBuffer& operator << (unsigned int v);
    LogBuffer& operator << (long v);
    LogBuffer& operator << (unsigned long v);
    LogBuffer& operator << (long long v);
    LogBuffer& operator << (unsigned long long v);

    LogBuffer& operator << (float v);
    LogBuffer& operator << (double);


    LogBuffer& operator<<(bool v)
    {
        m_data.append(v ? "true" : "false");
        return *this;
    }

    LogBuffer& operator << (char c)
    {
      m_data.push_back(c);
      return *this;
    }

    LogBuffer& operator << (const std::string &str)
    {
        append(str.c_str(), str.size());
        return *this;
    }

	LogBuffer& operator << (const char * str)
	{
        m_data.insert(m_data.size(), str);
		return *this;
	}

    void append(const char *buf, int len)
    {
        m_data.insert(m_data.end(), buf, buf + len);
    }

    void append(const char *str)
    {
        m_data.insert(m_data.size(), str);
    }


    const char* data()const { return m_data.c_str(); }
    std::string::size_type size()const { return m_data.size(); }

private:
    std::string m_data;
};

}

#endif // LOGBUFFER_HPP
