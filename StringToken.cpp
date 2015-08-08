//Filename: StringToken.cpp
//Date: 2015-5-25

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"StringToken.hpp"



namespace ADNS
{

namespace Utility
{

//if m_pos points to non separator char, this
//function still find separator and jum over
static std::string::size_type
       jumpOverSeparator(const std::string &str, const std::string &sep,
                         std::string::size_type pos)
{
    decltype(pos) curr, last;
    last = str.find(sep, pos);//find first separator after m_pos

    //assume m_sep is "\r\n", m_str may be
    //"\r\n\r\nabcde\r\nru3o"
    while( (curr = str.find(sep, last)) != std::string::npos)
    {
        if( curr != last )
            break;
        else
            last = curr + sep.size();
    }

    pos = last;

    //m_str may be "\r\n\r\n"
    if(pos >= str.size() )
        pos = std::string::npos;

    return pos;
}


//##################################


StringToken::StringToken(const std::string &str, const std::string &separator)
    : m_str(str),
      m_sep(separator),
      m_pos(0)
{
    reset();//set m_pos
}


void StringToken::setSeparator(const std::string &separator)
{
    m_sep = separator;
}



void StringToken::reset()
{
    if(m_str.empty())
        m_pos = std::string::npos;
    else if(m_str.find(m_sep, 0) == 0)
    {
        m_pos = jumpOverSeparator(m_str, m_sep, m_pos);
    }
}



std::string StringToken::next()
{
    std::string ret;//for RVO
    if( hasNext() )
    {
        auto next_pos = m_str.find(m_sep, m_pos);

        ret = m_str.substr(m_pos, next_pos - m_pos);

        m_pos = jumpOverSeparator(m_str, m_sep, m_pos);
    }

    return ret;
}


StringToken::iterator StringToken::begin()
{
    decltype(m_pos) pos = 0;

    if(m_str.empty())
        pos = std::string::npos;
    else if(m_str.find(m_sep, 0) == 0)
    {
        pos = jumpOverSeparator(m_str, m_sep, pos);
    }

    StringToken::iterator it(*this, pos);

    return it;
}


StringToken::iterator StringToken::end()
{
    StringToken::iterator it(*this, std::string::npos);

    return it;
}



StringToken::TokenList StringToken::splitTokens(const std::string &str, const std::string &separator)
{
    StringToken tmp(str, separator);

    TokenList tokens;
    while(tmp.hasNext())
        tokens.push_back(tmp.next());

    return tokens;
}


//#######################################


StringToken_Iterator::StringToken_Iterator(const StringToken &str_token,
                                           std::string::size_type pos)
    : m_str_token(str_token),
      m_pos(pos)
{}





StringToken_Iterator& StringToken_Iterator::operator ++()
{
    m_pos = jumpOverSeparator(m_str_token.m_str, m_str_token.m_sep, m_pos);
    return *this;
}


StringToken_Iterator StringToken_Iterator::operator ++(int )
{
    auto tmp = *this;
    m_pos = jumpOverSeparator(m_str_token.m_str, m_str_token.m_sep, m_pos);

    return tmp;
}


std::string StringToken_Iterator::operator *()
{
    if( m_pos == std::string::npos)
        return "";

    auto next_pos = m_str_token.m_str.find(m_str_token.m_sep, m_pos);

    return m_str_token.m_str.substr(m_pos, next_pos - m_pos);
}




}



}

