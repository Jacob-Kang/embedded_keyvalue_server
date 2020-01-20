#ifndef __NET_H
#define __NET_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "client.h"

#define IP_STR_LEN 46
class Net_util {
 public:
  Net_util(int port, std::string ip) : port_(port), ip_(ip) {}
  ~Net_util() {}
  void tcpConnect();
  void tcpRecv();
  void tcpSend();
  struct msg *querybuf_;

 private:
  int port_;
  std::string ip_;
  int cfd_;
};
#endif