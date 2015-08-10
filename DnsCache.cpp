//Filename: DnsCache.cpp
//Date: 2015-7-26

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"DnsCache.hpp"


namespace ADNS
{


RecordData DnsCache::query(const DnsQuery_t &query)
{
    return m_domain_tree.queryRecord(query.domain, query.query_type);
}


void DnsCache::flush(const DnsQueryResult_t &result)
{
    m_domain_tree.addRecord(result.domain, result.query_type, result.record_data, result.ttl);
}

}
