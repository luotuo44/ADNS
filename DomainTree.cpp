//Filename: DomainTree.cpp
//Date: 2015-7-11

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#include"DomainTree.hpp"

#include<assert.h>

#include<map>
#include<algorithm>
#include<iostream>
#include<chrono>

#include"StringToken.hpp"
#include"typedef.hpp"

namespace ADNS
{

struct Record
{
    DNSQueryType type;
    std::chrono::steady_clock::time_point start_point;
    int ttl;

    RecordData data;
};

typedef std::map<DNSQueryType, Record> RecordMap;
typedef std::shared_ptr<RecordMap> RecordMapPtr;

struct DomainNode
{
    //a leaf node means that a domain's zone token end in this node
    bool leaf;
    std::string zone;
    std::map<std::string, DomainNodePtr> branch;

    DomainNode(std::string z)
        : leaf(false),
          zone(std::move(z))
    {}

    //if leaf is false, records should points to nothing
    RecordMapPtr records;
};


DomainTree::DomainTree()
    : m_root(std::make_shared<DomainNode>("."))
{
}


DomainTree::~DomainTree()
{
    clearTree();
}

//DomainTree::DomainTree(const DomainTree& tree)
//    : m_root(std::make_shared<DomainNode>("."))
//{
//    copyTree(m_root, tree.m_root);
//}


//DomainTree& DomainTree::operator = (const DomainTree& tree)
//{
//    if( this == &tree)
//        return *this;

//    clearTree();
//    copyTree(m_root, tree.m_root);
//}


void DomainTree::copyTree(DomainNodePtr &dst, const DomainNodePtr &src)
{
    assert(dst && src && dst->branch.empty());

    auto it = src->branch.begin();
    for(; it != src->branch.end(); ++it)
    {
        auto iter = dst->branch.insert(dst->branch.begin(), {it->first, std::make_shared<DomainNode>(it->first)});
        iter->second->leaf = it->second->leaf;
        copyTree(iter->second, it->second);
    }
}

void DomainTree::swap(DomainTree &tree)
{
    m_root.swap(tree.m_root);
}


void DomainTree::clearTree()
{
    DomainNodePtr(std::make_shared<DomainNode>(".")).swap(m_root);
}


//this template is only used for Utility::StringToken::TokenList
//I don't want the include StringToken.hpp in the header file,
//so I use this method.
//If domain tree contains the domain that determined by b and e,
//this function return e, and node parameter points to the Node
//that contains the last token for the domain.
//If domain tree don't contains the domain. the return value is
//the first token that exclude in the domain tree, and parameter node
//points to the Node that contains the *--b.
template<typename Iter>
Iter DomainTree::findNode(Iter b, Iter e, DomainNodePtr &node)
{
    for(; b != e; ++b)
    {
        auto node_it = node->branch.find(*b);
        if( node_it == node->branch.end() )
            break;
        else
            node = node_it->second;
    }

    return b;
}


void DomainTree::addRecord(const std::string &domain, DNSQueryType type, const RecordData &data)
{
    assert(!domain.empty() && !data.empty());

    Utility::StringToken::TokenList tokens =  Utility::StringToken::splitTokens(domain, ".");
    tokens.reverse();
    DomainNodePtr node = m_root;
    auto it = findNode(tokens.begin(), tokens.end(), node);

    assert(node);
    for(; it != tokens.end(); ++it)
    {
        auto iter = node->branch.insert(node->branch.begin(), {*it, std::make_shared<DomainNode>(*it)});
        node = iter->second;
    }

    if( !node->leaf )
    {
        node->leaf = true;
        node->records = std::make_shared<RecordMap>();
    }

    assert(node->records);
    Record re;
    re.type = type;
    re.data = data;
    (*node->records)[type] = std::move(re);
}


//if no record, the returned vector will be empty
RecordData DomainTree::queryRecord(const std::string &domain, DNSQueryType type)
{
    Utility::StringToken::TokenList tokens =  Utility::StringToken::splitTokens(domain, ".");
    tokens.reverse();

    DomainNodePtr node = m_root;
    auto it = findNode(tokens.begin(), tokens.end(), node);

    RecordMap::iterator iter;
    if( it != tokens.end() || !node->leaf || (iter = node->records->find(type)) == node->records->end())
        return RecordData();
    else
        return iter->second.data;
}




void DomainTree::addDomain(const std::string &domain)
{
    assert(!domain.empty());

    Utility::StringToken::TokenList tokens =  Utility::StringToken::splitTokens(domain, ".");
    tokens.reverse();
    DomainNodePtr node = m_root;
    auto it = findNode(tokens.begin(), tokens.end(), node);
    if( it == tokens.end() )//the tree has this domain
        return ;

    assert(node);
    for(; it != tokens.end(); ++it)
    {
        auto iter = node->branch.insert(node->branch.begin(), {*it, std::make_shared<DomainNode>(*it)});
        node = iter->second;
    }

    assert(node);
    node->leaf = true;
}


using TokenListIter = Utility::StringToken::TokenList::iterator;

//return true means need to del sub branch
bool doDelDomain(TokenListIter it, TokenListIter end, DomainNodePtr &node)
{
    if( it == end && node->branch.empty() )
        return true;
    else if( it == end )
    {
        node->leaf = false;//del it
        return false;
    }

    auto node_it = node->branch.find(*it);
    if( node_it == node->branch.end() )//this zone doesn't exist
        return false;

    if( doDelDomain(++it, end, node_it->second) )//need to del
        node->branch.erase(node_it);//del sub branch node

    //this node doesn't has sub branch, and this node isn't leaf
    return node->branch.empty() && !node->leaf;
}


void DomainTree::delDomain(const std::string &domain)
{
    assert(!domain.empty());

    Utility::StringToken::TokenList tokens =  Utility::StringToken::splitTokens(domain, ".");
    tokens.reverse();

    doDelDomain(tokens.begin(), tokens.end(), m_root);
}



bool DomainTree::hasDomain(const std::string &domain)
{
    assert(!domain.empty());

    Utility::StringToken::TokenList tokens =  Utility::StringToken::splitTokens(domain, ".");
    tokens.reverse();

    DomainNodePtr node = m_root;
    auto it = tokens.begin();
    for(; it != tokens.end(); ++it)
    {
        auto node_it = node->branch.find(*it);
        if(node_it == node->branch.end())
            break;
        else
            node = node_it->second;
    }

    return it == tokens.end() && node->leaf;
}



void doPrintfTree(const DomainNodePtr &node, std::list<std::string> &zones)
{
    if( node->leaf )
    {
        std::for_each(zones.rbegin(), zones.rend(), [](const std::string &zone){
            std::cout<<zone<<'.';
        });

        std::cout<<"\b "<<std::endl;
    }

    for(auto it = node->branch.begin(); it != node->branch.end(); ++it)
    {
        zones.push_back(it->first);
        doPrintfTree(it->second, zones);
        zones.pop_back();
    }
}


void DomainTree::printfTree()const
{
    std::list<std::string> zones;
    doPrintfTree(m_root, zones);
}


}
