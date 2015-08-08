//Filename: DNS.cpp
//Date: 2015-2-20

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#include"DNS.hpp"

#include<arpa/inet.h>
#include<netinet/in.h>

#include<string.h>
#include<assert.h>
#include<stdint-gcc.h>
#include<stdio.h>


#include"SocketOps.hpp"

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

enum qtype
{
    A = 1,
    NS = 2,
    CNAME = 5,
    PTR = 12,
    HINFO = 13,
    MX = 15
};


#pragma pack(push, 1)
typedef struct ANSWER_tag
{
    uint16_t qtype;
    uint16_t qclass;
    uint32_t ttl;
    uint16_t data_len;

    unsigned char data[];
}ANSWER_t;
#pragma pack(pop)

std::string getDNSPacket(int id, DNSQueryType type, const std::string &hostname)
{
    char buff[1024];

    DNS_HEADER_t *head = (DNS_HEADER_t*)buff;


    head->id = Net::SocketOps::htons(id);

    head->flag.field.qr = 0;
    head->flag.field.opcode = 0;
    head->flag.field.aa = 0;
    head->flag.field.tc = 0;
    head->flag.field.rd = 1;

    head->flag.field.ra = 0;
    head->flag.field.zero = 0;
    head->flag.field.rcode = 0;

    head->flag.val = Net::SocketOps::htons(0x0100);

    head->q_count = Net::SocketOps::htons(1);
    head->a_count = 0;
    head->auth_count = 0;
    head->add_count = 0;


    std::string ret = toDNSFormat(hostname);

    int pos = sizeof(DNS_HEADER_t);
    memcpy(buff+pos, ret.c_str(), ret.size());
    //strncpy(buff+pos, ret.c_str(), ret.size());
    pos += ret.size();

    DNS_QUESTION_t *question_type = (DNS_QUESTION_t*)(buff+pos);

    question_type->qtype = Net::SocketOps::htons(static_cast<int>(type));
    question_type->qclass = Net::SocketOps::htons(1);
    pos += sizeof(DNS_QUESTION_t);

    return std::string(buff, buff+pos);
}

std::string toDNSFormat(std::string hostname)
{
    hostname += ".";//追加一个'.'，方便循环

    std::string dst;

    std::string::size_type pos = 0, last = 0;
    while( (pos = hostname.find('.', last) ) != std::string::npos)
    {
        dst += static_cast<char>(pos-last);
        dst.insert(dst.end(), hostname.begin()+last,
                   hostname.begin() + pos);

        last = pos + 1;
    }

    dst += static_cast<char>(0);//最后以0表示结束

    return dst;
}


std::string parseDNSFormat(const unsigned char *buff,
                           const unsigned char *curr,
                           int *count)
{
    int num = 0;
    int len;
    int offset;
    int jump = 0;
    std::string hostname;

    len = curr[0];

    const unsigned char *p = curr + 1;

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
        if( count != NULL && jump == 1)//但只计算第一次跳转的字节
            *count = num + 2;//两个跳转字节

        //这里不使用ntohs因为不够两个字节
        offset = ((p[-1]&0x3F)<<8) + *p;//0x3F = 0011 1111
        p = buff + offset;
        len = *p++;
        goto label;
    }

    ++num;
    if( count != NULL && !jump)//没有跳转的时候才用这里的计数
        *count = num;

    //hostname.pop_back();//删除最后面的"."
    hostname.erase(hostname.end()-1);

    return hostname;
}



std::string parseOneRecord(const unsigned char *buff,
                           const unsigned char *curr,
                           int *count, int *type)
{
    assert( type != NULL);

    ANSWER_t *answer = (ANSWER_t*)curr;
    int num = sizeof(ANSWER_t) + Net::SocketOps::ntohs(answer->data_len);


    std::string name;

    *type = Net::SocketOps::ntohs(answer->qtype);

    if( *type == CNAME)//CNAME
    {
        name = parseDNSFormat(buff, answer->data, NULL);
    }
    else if( *type == A)//A
    {
        name = parseIP(answer->data);
    }
    else //其他类型不进行解析
    {
        name = "unknown result";
    }


    if( count != NULL )
        *count = num;

    return name;
}



//buff jump the length message in the tcp-dns packet
std::vector<std::string>
        parseDNSResultPacket(const unsigned char *buff, int len)
{
    (void)len;

    std::vector<std::string> str_vec;
    DNS_HEADER_t *head = (DNS_HEADER_t*)buff;

    int answer_num = Net::SocketOps::ntohs(head->a_count);

    if(answer_num <= 0 ) //doesn't have answer
        return str_vec;


    const unsigned char *query_start = buff + sizeof(DNS_HEADER_t);
    int count;
    std::string hostname = parseDNSFormat(buff, query_start, &count);

    //query type and query class
    const unsigned char *answer_start = query_start + count + 2+2;

    int type;

    assert(sizeof(ANSWER_t) == 10);

    do
    {   //answer domain
        parseDNSFormat(buff, answer_start, &count);

        answer_start += count;
        std::string data = parseOneRecord(buff, answer_start,
                                          &count, &type);

        if( type == CNAME )
        {
            //do nothing
        }
        else if( type == A)
        {
            str_vec.push_back(data);
        }
        else
        {
            fprintf(stderr, "unknown result\n");
        }

        answer_start += count;

    }while(--answer_num );

    return str_vec;
}


std::string parseIP(const unsigned char *buf)
{
    const unsigned int *p = reinterpret_cast<const unsigned int*>(buf);

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = *p;
    return ::inet_ntoa(addr.sin_addr);
}

}

}
