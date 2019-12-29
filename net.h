#ifndef __NET_H
#define __NET_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include "server.h"

int tcpConnect(int port);
int tcpAccept(int server_fd);
void tcpRecv(int cfd);
void tcpSend(int cfd, char *msg, int msg_len);
// int tcpAccept(void* arg);

#endif