//Filename: DnsClient.hpp
//Date: 2015-7-16

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DNSCLIENT_HPP
#define DNSCLIENT_HPP

#include<string>
#include<memory>
#include<functional>

#include"typedef.hpp"

namespace ADNS
{


class DnsClient;
using DnsClientPtr = std::shared_ptr<DnsClient> ;


class DnsClient
{
public:
    ~DnsClient()=default;
    DnsClient(DnsClient &&)=default;
    DnsClient& operator = (DnsClient &&)=default;


    bool addQuery(const DnsQuery_t &query);
    bool addQuery(DnsQuery_t &&query);

    static void init();//isn't threadsafe
    static DnsClientPtr getInstance();//threadsafe

    class Impl;
    friend class Impl;
    using ImplPtr = std::shared_ptr<Impl>;

private:
    DnsClient()=default;
    DnsClient(const DnsClient&)=delete;
    DnsClient& operator = (const DnsClient&)=delete;

private:
    bool isLegalQuery(const DnsQuery_t &query);


private:
    ImplPtr m_impl;
    static DnsClientPtr dns_client;
};


}


#endif // DNSEXPLORER_HPP
