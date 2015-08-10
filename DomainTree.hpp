//Filename: DomainTree.hpp
//Date: 2015-7-11

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef DOMAINTREE_HPP
#define DOMAINTREE_HPP

#include<string>
#include<memory>
#include<list>

#include"typedef.hpp"

namespace ADNS
{


struct DomainNode;
typedef std::shared_ptr<DomainNode> DomainNodePtr;


class DomainTree
{
public:
    DomainTree();
    ~DomainTree();

    DomainTree(const DomainTree &tree)=delete;
    DomainTree& operator = (const DomainTree &tree)=delete;


    void swap(DomainTree &tree);

    void addRecord(const std::string &domain, DNSQueryType type, const RecordData &data, int ttl);
    //if no record, the returned vector will be empty
    RecordData queryRecord(const std::string &domain, DNSQueryType type)const;
    void addDomain(const std::string &domain);
    void delDomain(const std::string &domain);
    bool hasDomain(const std::string &domain);
    void clearTree();
    void printfTree()const;

private:
    void copyTree(DomainNodePtr &dst, const DomainNodePtr &src);
    //bool doDelDomain();


    template<typename Iter>
    static Iter findNode(Iter b, Iter e, DomainNodePtr &node);

private:
    DomainNodePtr m_root;
};


}


#endif // DOMAINTREE_HPP
