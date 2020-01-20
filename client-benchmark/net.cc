#include "net.h"
void Net_util::tcpConnect() {
  struct sockaddr_in server_addr, client_addr;
  if ((cfd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {  // 소켓 생성
    printf("[NET] Can't open stream socket.");
    exit(-1);
  }
  memset(&server_addr, 0x00, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip_.c_str());
  server_addr.sin_port = htons(port_);
  int ret = connect(cfd_, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (ret < 0) {
    printf("[NET] Can't connect port #%d.", port_);
    exit(-1);
  }
  querybuf_ = (struct msg *)malloc(sizeof(struct msg) + KV_IOBUF_LEN);
  std::cout << "[NET] Connect " << ip_ << ":" << port_ << std::endl;
}

void Net_util::tcpRecv() {
  int rst;
  rst = read(cfd_, querybuf_->buf, KV_IOBUF_LEN);
  if (rst < 0) printf("recv error");
  printf("[NET] MSG Recived\n%s", querybuf_->buf);
}
void Net_util::tcpSend() {
  int rst;
  rst = write(cfd_, querybuf_->buf, querybuf_->len);
  if (rst < 0) printf("recv error");
  printf("[NET] MSG Recived\n%s", querybuf_->buf);
}
