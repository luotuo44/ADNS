//Filename: DNS.hpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DNS_HPP
#define DNS_HPP

#include<string>
#include<vector>


#include"typedef.hpp"

namespace ADNS
{

namespace DNS
{

std::string getDNSPacket(int id, DNSQueryType type, const std::string &hostname);


//buff jump the length message in the tcp-dns packet
std::vector<std::string>  parseDNSResultPacket(const char *buff, int len, uint32_t &ttl);

std::string parseIP(const char *buf);

}

}
#endif // DNS_HPP
