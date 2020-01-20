#include "client.h"
struct kvClient client;
static void sigShutdownHandler(int sig) {
  std::string msg;
  switch (sig) {
    case SIGINT:
      msg = "Received SIGINT scheduling shutdown...";
      break;
    case SIGTERM:
      msg = "Received SIGTERM scheduling shutdown...";
      break;
    default:
      msg = "Received shutdown signal, scheduling shutdown...";
  };
  std::cout << msg << std::endl;
  if (sig == SIGINT) {
    std::cout << "You insist... exiting now." << std::endl;
    exit(1);
  }
}
int setupSignalHandlers(void) {
  sighandler_t sig_ret;
  sig_ret = signal(SIGTERM, sigShutdownHandler);
  if (sig_ret == SIG_ERR) {
    std::cout << "can't set SIGTERM handler" << std::endl;
    return EXIT_FAILURE;
  }
  signal(SIGINT, sigShutdownHandler);
  if (sig_ret == SIG_ERR) {
    std::cout << "can't set SIGINT handler" << std::endl;
    return EXIT_FAILURE;
  }
  return 0;
}

static void cliRefreshPrompt(void) {
  int len;
  client.prompt.clear();
  client.prompt +=
      client.ip.append(":") + std::to_string(client.port).append("> ");
}

void initClient(void) {
  Net_util net(client.port, client.ip);
  net.tcpConnect();
  client.cli = static_cast<void*>(&net);
  cliRefreshPrompt();
}
int main(int argc, char** argv) {
  if (argc >= 2) {
    char* configfile = NULL;
    configfile = argv[1];
    loadServerConfig(configfile);
  } else {
    std::cout << "ex: " << argv[0] << " conf/client.conf" << std::endl;
    exit(-1);
  }
  initClient();
  std::cout << client.prompt;
  int data;
  std::cin >> data;
  return 0;
}