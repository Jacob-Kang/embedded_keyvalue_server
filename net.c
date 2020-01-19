#include "net.h"
int tcpConnect(int port) {
  int server_fd;
  char ip[IP_STR_LEN];

  struct sockaddr_in server_addr, client_addr;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {  // 소켓 생성
    chLog(LOG_ERROR, "[NET] Can't open stream socket.");
    exit(-1);
  }
  memset(&server_addr, 0x00, sizeof(server_addr));
  // server_Addr 을 NULL로 초기화

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);
  // server_addr 셋팅

  int ret =
      bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  while (ret < 0) {
    chLog(LOG_ERROR, "[NET] Can't bind #%d port.", port);
    server_addr.sin_port = htons(++port);
    ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  }
  // if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
  //     0) {  // bind() 호출
  //   chLog(LOG_ERROR, "[NET] Can't bind local address.");
  //   exit(-1);
  // }
  if (listen(server_fd, 5) < 0) {  //소켓을 수동 대기모드로 설정
    chLog(LOG_ERROR, "Can't listening connect.");
    exit(-1);
  }
  //   int flags;
  //   flags |= O_NONBLOCK;
  //   if (fcntl(server_fd, F_SETFL, flags) == -1) {
  //     chLog(LOG_ERROR, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
  //     exit(-1);
  //   }
  inet_ntop(AF_INET, &server_addr.sin_addr.s_addr, ip, sizeof(ip));
  chLog(LOG_NOTICE, "[NET] Running %s:%d", ip, port);
  return server_fd;
}

int tcpAccept(int server_fd) {
  // int tcpAccept(void *arg) {
  // int server_fd = (int)arg;
  struct sockaddr_in client_addr;
  char ip[IP_STR_LEN];
  int port;
  socklen_t len = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
  if (client_fd == -1) {
    if (errno == EINTR)
      return 0;
    else {
      chLog(LOG_ERROR, "[NET] Accept failed: %s", strerror(errno));
      exit(-1);
    }
  }
  port = ntohs(client_addr.sin_port);
  inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip, sizeof(ip));
  chLog(LOG_NOTICE, "[NET] Accepted %s:%d", ip, port);
  return client_fd;
}

int tcpRecv(struct kvClient *c) {
  int rst;
  rst = read(c->fd, c->querybuf->buf, KV_IOBUF_LEN);
  if (rst < 0) chLog(LOG_ERROR, "[%d] recv error", c->fd);
  return rst;
  // chLog(LOG_DEBUG, "[NET] %d: MSG Recived\n%s", c->fd, c->querybuf->buf);
}

void tcpSend(struct kvClient *c) {
  int rst;
  rst = write(c->fd, c->querybuf->buf, c->querybuf->len);
  if (rst <= 0) chLog(LOG_ERROR, "[%d] send error", c->fd);
  // chLog(LOG_DEBUG, "[NET] %d: MSG Send\n%s", c->fd, c->querybuf->buf);
}
