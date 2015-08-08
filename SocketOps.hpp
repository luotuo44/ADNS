//Filename: SocketOps.hpp
//Date: 2015-6-13

//Author: luotuo44   http://blog.csdn.net/luotuo44

//Copyright 2015, luotuo44. All rights reserved.
//Use of this source code is governed by a BSD-style license

#ifndef SOCKETOPS_HPP
#define SOCKETOPS_HPP

#include<stdint-gcc.h>

namespace ADNS
{

namespace Net
{

namespace SocketOps
{

uint16_t htons(uint16_t host);
uint16_t ntohs(uint16_t net);

uint32_t htonl(uint32_t host);
uint32_t ntohl(uint32_t net);

int new_tcp_socket();
void close_socket(int fd);
int make_nonblocking(int fd);

//-1 system call error. 0 connect success. 1 wait to connect,
//cannot connect immediately, and need to try again
int tcp_connect_server(const char *server_ip, int port, int sockfd);

bool new_pipe(int fd[2], bool r_nonblocking, bool w_nonblocking);

int new_tcp_socket_connect_server(const char* server_ip, int port, int *sockfd);

int connecting_server(int fd);

bool wait_to_connect(int err);
bool refuse_connect(int err);


int write(int fd, char *buf, int len);
int read(int fd, char *buf, int len);

int writen(int fd, const char *buf, int n);
int readn(int fd, char *buf, int n);

bool isLegalIP(const char *ip);

}

}

}

#endif // SOCKETOPS_HPP
