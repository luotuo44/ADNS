//Filename: DNS.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#include"DNS.hpp"

#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/types.h>

#include<string.h>
#include<assert.h>
#include<stdint-gcc.h>
#include<stdio.h>


#include"SocketOps.hpp"
#include"typedef.hpp"

namespace ADNS
{

namespace DNS
{


typedef struct DNS_SUB_HEADER_tag
{
    uint8_t rd:1;
    uint8_t tc:1;
    uint8_t aa:1;
    uint8_t opcode:4;
    uint8_t qr:1;

    uint8_t rcode:4;
    uint8_t zero:3;
    uint8_t ra:1;
}DNS_SUB_HEADER_t;

typedef struct DNS_HEADER_tag
{
    uint16_t id;

    union
    {
        //bit-fields is importable
        DNS_SUB_HEADER_t field;
        uint16_t val;
    }flag;

    uint16_t q_count;//question count
    uint16_t a_count;//answer count

    uint16_t auth_count;
    uint16_t add_count;

}DNS_HEADER_t;


typedef struct DNS_QUESTION_tag
{
    uint16_t qtype;
    uint16_t qclass;
}DNS_QUESTION_t;


#pragma pack(push, 1)
typedef struct ANSWER_tag
{
    uint16_t qtype;
    uint16_t qclass;
    uint32_t ttl;
    uint16_t data_len;

    char data[];
}ANSWER_t;
#pragma pack(pop)


void appendHeaderPacket(DNS_HEADER_t *header, uint16_t id)
{
    assert(header != nullptr);

    header->id = Net::SocketOps::htons(id);

    {
        header->flag.field.qr = 0;
        header->flag.field.opcode = 0;
        header->flag.field.aa = 0;
        header->flag.field.tc = 0;
        header->flag.field.rd = 1;

        header->flag.field.ra = 0;
        header->flag.field.zero = 0;
        header->flag.field.rcode = 0;

        header->flag.val = Net::SocketOps::htons(0x0100);
    }

    //just support only one question on a query
    //http://forums.devshed.com/dns/183026-dns-packet-question-section-1-a-post793792.html#post793792
    //http://stackoverflow.com/questions/4082081/requesting-a-and-aaaa-records-in-single-dns-query/4083071#4083071
    header->q_count = Net::SocketOps::htons(1);
    header->a_count = 0;
    header->auth_count = 0;
    header->add_count = 0;
}

std::string toDNSFormat(std::string hostname)
{
    hostname += ".";//append one ".", convenient for loop

    std::string dst;
    dst.reserve(hostname.size() + 4);
    std::string::size_type pos = 0, last = 0;
    while( (pos = hostname.find('.', last) ) != std::string::npos)
    {
        dst += static_cast<char>(pos-last);
        dst.insert(dst.end(), hostname.begin()+last, hostname.begin() + pos);
        last = pos + 1;
    }

    dst += static_cast<char>(0);//end with 0

    return dst;
}


int appendQuestionPacket(char *buff, size_t left, DNSQueryType type, const std::string &hostname)
{
    std::string dns_format_domain = toDNSFormat(hostname);

    size_t need_size = dns_format_domain.size() + sizeof(DNS_QUESTION_t);
    if( need_size <= left)
    {
        memcpy(buff, dns_format_domain.c_str(), dns_format_domain.size());

        DNS_QUESTION_t *question = reinterpret_cast<DNS_QUESTION_t*>(buff+dns_format_domain.size());
        question->qtype = Net::SocketOps::htons(static_cast<int>(type));
        question->qclass = Net::SocketOps::htons(1);
    }

    return need_size <= left ? need_size : -1;
}


std::string getDNSPacket(int id, DNSQueryType type, const std::string &hostname)
{
    constexpr int MAX_SIZE = 1024;
    char buff[MAX_SIZE];

    DNS_HEADER_t *header = (DNS_HEADER_t*)buff;
    appendHeaderPacket(header, id);

    constexpr int left = MAX_SIZE - sizeof(DNS_HEADER_t);
    int len = appendQuestionPacket(buff + sizeof(DNS_HEADER_t), left, type, hostname);

    if( len == -1 )
        return std::string();
    else
        return std::string(buff, buff + sizeof(DNS_HEADER_t) + len);
}



std::string parseDNSFormat(const char *buff, const char *curr, int *count)
{
    int num = 0;
    int len;
    int offset;
    int jump = 0;
    std::string hostname;

    const unsigned char *start = reinterpret_cast<const unsigned char*>(buff);
    const unsigned char *p = reinterpret_cast<const unsigned char*>(curr);
    len = p[0];

    ++p;

label:

    while( len != 0 && len < 192)//192是以11开头的字节
    {
        ++num;
        while(len--)
        {
            ++num;
            hostname += *p++;
        }

        hostname += '.';

        len = *p++;
    }


    if( len >= 192 )
    {
        ++jump;//可能会多次跳转
        if( count != nullptr && jump == 1)//但只计算第一次跳转的字节
            *count = num + 2;//两个跳转字节

        //这里不使用ntohs因为不够两个字节
        offset = ((p[-1]&0x3F)<<8) + *p;//0x3F = 0011 1111
        p = start + offset;
        len = *p++;
        goto label;
    }

    ++num;
    if( count != nullptr && !jump)//没有跳转的时候才用这里的计数
        *count = num;

    assert(!hostname.empty());
    //hostname.pop_back();//删除最后面的"."
    hostname.erase(hostname.end()-1);

    return hostname;
}



std::string parseOneRecord(const char *buff, const char *curr, int &count, int &type, uint32_t &ttl)
{
    static_assert(sizeof(ANSWER_t) == 10, "sizeof(ANSWER_t) should equals to 10");

    const ANSWER_t *answer = reinterpret_cast<const ANSWER_t*>(curr);
    count = sizeof(ANSWER_t) + Net::SocketOps::ntohs(answer->data_len);
    ttl = Net::SocketOps::ntohl(answer->ttl);
    type = Net::SocketOps::ntohs(answer->qtype);

    int mx_step = 0;
    std::string name;
    switch(static_cast<DNSQueryType>(type) )
    {
    case DNSQueryType::A:
        name = parseIP(answer->data);
        break;

    case DNSQueryType::MX: //fall
        mx_step = 2;//mx record has 16 bit preference
    case DNSQueryType::NS: //fall
    case DNSQueryType::CNAME:
        name = parseDNSFormat(buff, answer->data + mx_step, nullptr);
        break;

    default://doesn't resolve other kind
        name = "unknown result";
        break;
    }

    return name;
}



//buff jump the length message in the tcp-dns packet
std::vector<std::string> parseDNSResultPacket(const char *buff, int len, uint32_t &ttl)
{
    int count, curr_type;
    std::vector<std::string> str_vec;
    const DNS_HEADER_t *header = reinterpret_cast<const DNS_HEADER_t*>(buff);

    int answer_num = Net::SocketOps::ntohs(header->a_count);
    if(answer_num <= 0 ) //doesn't have answer
        return str_vec;


    const char *query_start = buff + sizeof(DNS_HEADER_t);
    parseDNSFormat(buff, query_start, &count);
    const DNS_QUESTION_t *question = reinterpret_cast<const DNS_QUESTION_t*>(query_start + count);
    int query_type = Net::SocketOps::ntohs(question->qtype);

    const char *answer_start = query_start + count + sizeof(DNS_QUESTION_t);
    do
    {   //answer domain
        parseDNSFormat(buff, answer_start, &count);

        answer_start += count;
        std::string data = parseOneRecord(buff, answer_start, count, curr_type, ttl);

        if( query_type == curr_type && !data.empty())
            str_vec.push_back(std::move(data));

        answer_start += count;

    }while(--answer_num );

    return str_vec;

    (void)len;
}


std::string parseIP(const char *buf)
{
    const int *p = reinterpret_cast<const int*>(buf);

    char dst[20];
    const char *ret = ::inet_ntop(AF_INET, p, dst, sizeof(dst));

    std::string str;
    if( ret != nullptr)
        str = ret;
    return str;
}

}

}
