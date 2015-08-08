//Filename: typedef-internal.hpp
//Date: 2015-7-28

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef TYPEDEFINTERNAL_HPP
#define TYPEDEFINTERNAL_HPP

#include<memory>
#include<functional>

#include"typedef.hpp"

namespace ADNS
{

namespace Net
{
class Event;
typedef std::shared_ptr<Event> EventPtr;


class Reactor;
typedef std::shared_ptr<Reactor> ReactorPtr;
typedef std::weak_ptr<Reactor> ReactorWPtr;

using EVENT_CB = std::function<void (int, int , void*)>;

}

using EventCreater = std::function<Net::EventPtr (int, int, Net::EVENT_CB , void *)>;

using DnsExplorerQueryFun = std::function<void (DnsQuery_t &&)>;
using DnsExplorerResCB = std::function<void (DnsQueryResult_t &&)>;

using DnsCacheSaveFun = std::function<void (const DnsQueryResult_t &)>;
using DnsCacheQueryFun = std::function<RecordData (const DnsQuery_t &)>;

using DnsClientResCB = std::function<void (DnsQueryResult_t &&)>;
}

#endif // TYPEDEFINTERNAL_HPP
