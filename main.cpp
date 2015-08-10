#include <iostream>
#include<atomic>
#include<chrono>
#include<thread>

#include"StringToken.hpp"
#include"DomainTree.hpp"
#include"DnsClient.hpp"
#include"typedef.hpp"
#include"MutexLock.hpp"
#include"Condition.hpp"

using namespace std;
using namespace ADNS;

std::atomic<int> g_query_num(0);
Mutex g_mutex;
Condition g_cond(g_mutex);

std::string dnsQueryTypeToString(DNSQueryType type)
{
    std::string name;
    switch(type)
    {
        case DNSQueryType::A:
            name = "A";
            break;

        case DNSQueryType::CNAME:
            name = "CNAME";
            break;

        case DNSQueryType::NS:
            name = "NS";
            break;

        case DNSQueryType::MX:
            name = "MX";
            break;

        default:
            name = "unknown";
        
    }

    return name;
}

void dnsCB(const DnsQueryResult_t &res)
{
    cout<<endl<<"===== "<<endl;
    if( !res.success)
    {
        cout<<"fail to query for "<<res.fail_reason<<endl;
    }
    else
    {
        cout<<res.dns_server<<" says "<<res.domain<<' '<<dnsQueryTypeToString(res.query_type)<<" record : "<<endl;
        for(auto &e : res.record_data)
            cout<<e<<'\t';
        cout<<endl;
    }

    --g_query_num;
    g_cond.notify();
}


int main()
{
    cout << "Hello World!" << endl;

    DnsClient::init();//init before using

    DnsClientPtr clientPtr = DnsClient::getInstance();

    DnsQuery_t query;
    query.cb = dnsCB;
    query.dns_server = "89.233.43.71";
    query.domain = "www.baidu.com";
    query.id = 34;
    query.query_type = DNSQueryType::A;
    query.query_procotol = DNSQueryProcotol::TCP;

    clientPtr->addQuery(query);
    ++g_query_num;

    query.domain = "www.qq.com";
    query.id = 56;
    clientPtr->addQuery(query);
    ++g_query_num;

    query.domain = "www.sina.com";
    query.id = 23;
    clientPtr->addQuery(query);
    ++g_query_num;


    query.domain = "www.qq.com";
    query.id = 28;
    query.query_type = DNSQueryType::CNAME;
    clientPtr->addQuery(query);
    ++g_query_num;

    query.domain = "qq.com";
    query.id = 344;
    query.query_type = DNSQueryType::MX;
    clientPtr->addQuery(query);
    ++g_query_num;

    std::this_thread::sleep_for(std::chrono::seconds(5));
    query.domain = "www.qq.com";
    query.id = 54;
    query.query_type = DNSQueryType::A;
    clientPtr->addQuery(query);
    ++g_query_num;


    query.domain = "qq.com";
    query.id = 444;
    query.query_type = DNSQueryType::NS;
    clientPtr->addQuery(query);
    ++g_query_num;


    MutexLock lock(g_mutex);
    while( g_query_num.load() > 0)
        g_cond.wait();

    return 0;
}

