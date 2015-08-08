//Filename: MinUnusedID.cpp
//Date: 2015-7-29

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#include"MinUnusedID.hpp"

#include<assert.h>
#include<algorithm>
#include<numeric>

namespace ADNS
{

namespace Utility
{


MinUnusedID::MinUnusedID(int limit)
    : m_limit(limit)
{
    assert(limit > 0);

    m_vec.reserve(limit);
    int i = 0;
    std::generate_n(std::back_inserter(m_vec), m_limit, [&i]()->int{
                        return i++;
                    });

    std::make_heap(m_vec.begin(), m_vec.end(), std::greater<int>());
}


int MinUnusedID::generateID()
{
    int id = -1;
    if( !m_vec.empty() )
    {
        id = m_vec.front();
        std::pop_heap(m_vec.begin(), m_vec.end(), std::greater<int>());
        m_vec.pop_back();
    }

    return id;
}


void MinUnusedID::releaseID(int id)
{
    assert( id >= 0 && id < m_limit);

    m_vec.push_back(id);
    std::push_heap(m_vec.begin(), m_vec.end());
}

}

}

