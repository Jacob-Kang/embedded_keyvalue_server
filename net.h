#ifndef __NET_H
#define __NET_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include "command.h"
#include "server.h"

int tcpConnect(int port);
int tcpAccept(int server_fd);
void tcpRecv(struct kvClient *c);
void tcpSend(struct kvClient *c);

#endif