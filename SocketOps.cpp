//Filename: SocketOps.cpp
//Date: 2015-6-13

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license


#include"SocketOps.hpp"

#include<unistd.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include<string.h>
#include<stdio.h>
#include<assert.h>
#include<errno.h>


#include"Logger.hpp"

namespace ADNS
{

namespace Net
{

namespace SocketOps
{

uint16_t htons(uint16_t host)
{
    return ::htons(host);
}

uint16_t ntohs(uint16_t net)
{
    return ::ntohs(net);
}


uint32_t htonl(uint32_t host)
{
    return ::htonl(host);
}


uint32_t ntohl(uint32_t net)
{
    return ::ntohl(net);
}


int new_tcp_socket()
{
    return ::socket(AF_INET, SOCK_STREAM, 0);
}

void close_socket(int fd)
{
    //ignore the error
    ::close(fd);
}

int make_nonblocking(int fd)
{
    int flags;
    if ((flags = ::fcntl(fd, F_GETFL, nullptr)) < 0)
        return -1;

    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return -1;

    return 0;
}


bool new_pipe(int fd[2], bool r_nonblocking, bool w_nonblocking)
{
    if( ::pipe(fd) == -1 )
        return false;

    bool ret = false;
    do
    {
        if( r_nonblocking && make_nonblocking(fd[0])==-1 )
            break;

        if( w_nonblocking && make_nonblocking(fd[1])==-1 )
            break;

        ret = true;
    }while(0);

    if(!ret)
    {
        ::close(fd[0]);
        ::close(fd[1]);
        fd[0] = -1;
        fd[1] = -1;
    }

    return ret;
}

typedef struct sockaddr SA;


//-1 system call error. 0 connect success. 1 wait to connect,
//cannot connect immediately, and need to try again
int tcp_connect_server(const char *server_ip, int port, int sockfd)
{
    int ret;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr) );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SocketOps::htons(port);
    ret = ::inet_aton(server_ip, &server_addr.sin_addr);
    if( ret == 0 ) //the server_ip is not valid value
    {
        errno = EINVAL;
        return -1;
    }

    ret = ::connect(sockfd, (SA*)&server_addr, sizeof(server_addr));

    if( ret == 0 )//connect success immediately
        return 0;
    else if( ret == -1)
    {
        if( SocketOps::wait_to_connect(errno) )
            return 1;
        else if(SocketOps::refuse_connect(errno) )
            LOG(Log::ERROR)<<"refuse connect";
        else
            LOG(Log::ERROR)<<"unknown error";
    }

    return -1;
}



int new_tcp_socket_connect_server(const char* server_ip, int port, int *sockfd)
{
    assert(sockfd != nullptr);

    do
    {
        *sockfd = SocketOps::new_tcp_socket();
        if( *sockfd == -1)
            return -1;

        if( SocketOps::make_nonblocking(*sockfd) == -1 )
            break;

        int ret = SocketOps::tcp_connect_server(server_ip, port, *sockfd);

        if( ret != -1 )
            return ret;

    }while(0);


    int save_errno = errno;
    SocketOps::close_socket(*sockfd);
    *sockfd = -1;
    errno = save_errno;

    return -1;
}



//-1 system call error. 0 connect success. 1 wait to connect,
int connecting_server(int fd)
{
    int err;
    socklen_t len = sizeof(err);

    if(::getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len) < 0)
        return -1;

    if( err == 0 ) //connect success
        return 0;

    if( wait_to_connect(err) )//try again
        return 1;

    errno = err;

    return -1;
}



bool wait_to_connect(int err)
{
    return (err == EINTR) || (err == EINPROGRESS);
}


bool refuse_connect(int err)
{
    return err == ECONNREFUSED;
}



//-1: system call error
//0: write 0 byte
//positive number : write bytes
int write(int fd, char *buf, int len)
{
    assert(len >= 0);

    int ret;
    int send_num = 0;

    while( len > 0 )
    {
        ret = ::write(fd, buf+send_num, len);
        if( ret > 0 )
        {
            send_num += ret;
            len -= ret;
        }
        else if( ret == 0)//socket fd fail
            return -1;
        else //ret < 0
        {
            if( errno == EINTR)
                continue;
            else if( errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return -1;
        }
    }

    return send_num;
}

int read(int fd, char *buf, int len)
{
    int ret;
    int recv_num = 0;

    while(len > 0)
    {
        ret = ::read(fd, buf+recv_num, len);
        if( ret > 0)
        {
            recv_num += ret;
            len -= ret;
        }
        else if(ret == 0)
            return -1;
        else //ret < 0
        {
            if( errno == EINTR)
                continue;
            else if( errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return -1;
        }
    }

    return recv_num;
}


int writen(int fd, const char *buf, int n)
{
    size_t      nleft;
    ssize_t     nwritten;
    const char  *ptr;

    ptr = buf;
    nleft = n;
    while (nleft > 0) {
        nwritten = ::write(fd, ptr, nleft);

        if ( nwritten <= 0 ) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0; /* and call write() again */
            else
                return -1; /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }

    return n;
}



int readn(int fd, char *buf, int n)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = buf;
    nleft = n;
    while (nleft > 0)
    {
        nread = ::read(fd, ptr, nleft);
        if( nread <= 0)
        {
            if( nread == 0)//EOF
                break;

            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return -1;
        }

        nleft -= nread;
        ptr   += nread;
    }

    return n - nleft;      /* return >= 0 */
}

bool isLegalIP(const char *ip)
{
    assert(ip != nullptr);

    const char *q = ip;
    int val = 0;
    int count = 0;
    while( *q != '\0'  )
    {
        if( count != 0  )
            ++q;

        val = 0;
        for(; *q != '.' && *q != '\0'; ++q)
        {
            if( !isdigit(*q)  )
                return false;

            val *= 10;
            val += *q - '0';
                
        }

        if( (val > 255) || (++count > 4) )
            return false;
        
    }

    return count == 4;
}



}//SocketOps

}//Net

}
