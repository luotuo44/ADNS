//Filename: DnsCache.hpp
//Date: 2015-7-16

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DNSCACHE_HPP
#define DNSCACHE_HPP


#include<string>
#include<list>
#include<memory>

#include"DomainTree.hpp"
#include"DnsClient.hpp"


namespace ADNS
{

class DnsCache
{
public:
    DnsCache()=default;
    ~DnsCache()=default;

    DnsCache(const DnsCache&)=delete;
    DnsCache& operator = (const DnsCache&)=delete;


    RecordData query(const DnsQuery_t &query);
    void flush(const DnsQueryResult_t &result);

private:
    DomainTree m_domain_tree;
};

using DnsCachePtr = std::shared_ptr<DnsCache> ;
using DnsCacheWPtr =  std::weak_ptr<DnsCache>;

}

#endif // DNSCACHE_HPP
