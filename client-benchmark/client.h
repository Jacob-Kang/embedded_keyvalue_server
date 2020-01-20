#ifndef __SERVER_H
#define __SERVER_H
#include <signal.h>
#include <stddef.h>
#include <sys/syscall.h>
#include "net.h"
#include "util.h"
#define KV_IOBUF_LEN 1024 * 40

typedef void (*sighandler_t)(int);

struct msg {
  unsigned int len;
  char buf[];
};

struct kvClient {
  /* Networking */
  int port;
  std::string ip;
  void *cli;
  std::string prompt;
};

extern struct kvClient client;
#endif