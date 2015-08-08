//Filename: typedef.hpp
//Date: 2015-7-16

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef TYPEDEF_HPP
#define TYPEDEF_HPP

#include<string>
#include<vector>
#include<functional>

namespace ADNS
{

enum class DNSQueryType
{
    A = 1,
    NS = 2,
    NAME = 5,
    MX = 15
};



enum class DNSQueryProcotol
{
    TCP,
    UDP
};

struct DnsQueryResult_t;
typedef std::function<void (const DnsQueryResult_t&)> DnsQueryCB;

using RecordData = std::vector<std::string> ;



struct DnsQueryResult_t
{
    DnsQueryResult_t()=default;
    ~DnsQueryResult_t()=default;
    DnsQueryResult_t(const DnsQueryResult_t&)=default;
    DnsQueryResult_t& operator = (const DnsQueryResult_t &)=default;
    DnsQueryResult_t(DnsQueryResult_t &&)noexcept = default;
    DnsQueryResult_t& operator = (DnsQueryResult_t &&)noexcept = default;

    int id;

    bool success;
    //if success, this value will set to "success".
    //if fail, it save the fail reason
    std::string fail_reason;

    std::string domain;
    std::string dns_server;

    RecordData record_data;
    DNSQueryType query_type;
    DnsQueryCB cb;


};


typedef std::function<void (const DnsQueryResult_t&)> DnsQueryCB;


struct DnsQuery_t
{
    DnsQuery_t()=default;
    ~DnsQuery_t()=default;
    DnsQuery_t(const DnsQuery_t&)=default;
    DnsQuery_t& operator = (const DnsQuery_t&)=default;
    DnsQuery_t(DnsQuery_t &&)noexcept =default;
    DnsQuery_t& operator = (DnsQuery_t &&)noexcept = default;

    int id;
    std::string domain;
    std::string dns_server;

    DNSQueryType query_type;
    DNSQueryProcotol query_procotol;

    DnsQueryCB cb;
};



}

#endif // TYPEDEF_HPP
