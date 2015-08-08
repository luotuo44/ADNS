//Filename: MinUnusedID.hpp
//Date: 2015-7-29

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef MINUNUSEDID_HPP
#define MINUNUSEDID_HPP

#include<vector>

namespace ADNS
{

namespace Utility
{

class MinUnusedID
{
public:
    MinUnusedID(int limit);
    ~MinUnusedID()=default;

    int generateID();
    void releaseID(int id);

private:
    int m_limit;
    std::vector<int> m_vec;
};

}

}

#endif // MINUNUSEDID_HPP
