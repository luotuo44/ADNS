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


std::string parseDNSFormat(const unsigned char *buff, const unsigned char *curr, int *count);
std::string parseOneRecord(const unsigned char *buff, const unsigned char *curr,
                           int *count, int *type);

//buff jump the length message in the tcp-dns packet
std::vector<std::string> //ignore the aliases
    parseDNSResultPacket(const unsigned char *buff, int len);

std::string parseIP(const unsigned char *buf);

}

}
#endif // DNS_HPP
