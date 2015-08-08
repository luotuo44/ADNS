//Filename: StringToken.hpp
//Date: 2015-5-25

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef STRINGTOKEN_HPP
#define STRINGTOKEN_HPP


#include<string>
#include<list>

namespace ADNS
{

namespace Utility
{


class StringToken_Iterator;


class StringToken
{
public:
    typedef StringToken_Iterator iterator;
    typedef std::list<std::string> TokenList;
    friend class StringToken_Iterator;

public:
    StringToken(const std::string &str, const std::string &separator);


    void setSeparator(const std::string &separator);

    void reset();
    bool hasNext() { return m_pos < m_str.size(); }
    std::string next();

    iterator begin();
    iterator end();

    static TokenList splitTokens(const std::string &str, const std::string &separator);

private:
    std::string m_str;
    std::string m_sep;//separator always be "\r\n"

    std::string::size_type m_pos;
};



class StringToken_Iterator
{
public:
    StringToken_Iterator(const StringToken &str_token, std::string::size_type pos);

    bool operator == (const StringToken_Iterator &rhs)const
    {
        return m_pos == rhs.m_pos;
    }

    bool operator != (const StringToken_Iterator &rhs)const
    {
        return m_pos != rhs.m_pos;
    }

    std::string operator*();

    StringToken_Iterator& operator++();//prefix version
    StringToken_Iterator operator++(int);//postfix version

private:
    const StringToken &m_str_token;
    std::string::size_type m_pos;
};


}

}

#endif // STRINGTOKEN_HPP
