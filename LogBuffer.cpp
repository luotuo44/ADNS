//Filename: LogBuffer.cpp
//Date: 2015-6-2

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"LogBuffer.hpp"
#include<algorithm>
#include<stddef.h>


namespace ADNS
{

LogBuffer::LogBuffer()
{
    m_data.reserve(120);
}




static const char digits[] = "9876543210123456789";
static const char* zero  = digits + 9;

template<typename T>
size_t convert(char* buf, T v) // make sure the buf can store the result
{
    T i = v;
    char* p = buf;

    do
    {
      int lsd = static_cast<int>(i % 10);
      i /= 10;
      *p++ = zero[lsd];
    } while (i != 0);

    if (v < 0)
    {
      *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}


LogBuffer& LogBuffer::operator << (short v)
{
    (*this)<<static_cast<int>(v);
    return *this;
}


LogBuffer& LogBuffer::operator << (unsigned short v)
{
    (*this)<<static_cast<unsigned int>(v);
    return *this;
}


LogBuffer& LogBuffer::operator << (int v)
{
    char buf[20];
    size_t len = convert(buf, v);
    append(buf, len);
    return *this;
}

LogBuffer& LogBuffer::operator << (unsigned int v)
{
    char buf[20];
    size_t len = convert(buf, v);
    append(buf, len);
    return *this;
}


LogBuffer& LogBuffer::operator << (long v)
{
    char buf[25];
    size_t len = convert(buf, v);
    append(buf, len);
    return *this;
}

LogBuffer& LogBuffer::operator << (unsigned long v)
{
    char buf[25];
    size_t len = convert(buf, v);
    append(buf, len);
    return *this;
}


LogBuffer& LogBuffer::operator << (long long v)
{
    char buf[25];
    size_t len = convert(buf, v);
    append(buf, len);
    return *this;
}


LogBuffer& LogBuffer::operator << (unsigned long long v)
{
    char buf[25];
    size_t len = convert(buf, v);
    append(buf, len);
    return *this;
}




LogBuffer& LogBuffer::operator << (float v)
{
    *this << static_cast<double>(v);
    return *this;
}


LogBuffer& LogBuffer::operator<<(double v)
{
    char buf[25];
    int len = sprintf(buf, "%.12g", v);
    append(buf, len);

    return *this;
}

}
