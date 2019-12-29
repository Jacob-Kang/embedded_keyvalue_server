#include "server.h"

struct kvServer server;

static void sigShutdownHandler(int sig) {
  char* msg;
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

  if (sig == SIGINT) {
    printf("You insist... exiting now.\n");
    exit(1);
  }
}
void setupSignalHandlers(void) {
  struct sigaction act;

  /* When the SA_SIGINFO flag is set in sa_flags then sa_sigaction is used.
   * Otherwise, sa_handler is used. */
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = sigShutdownHandler;
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  return;
}

void initServer(void) {
  server.pid = getpid();
  setupSignalHandlers();
  // /* Open the TCP listening socket for the user commands. */
  server.ipfd = tcpConnect(server.port);
}

void processEvents() {
  //  server.cli_fd = tcpAccept(server.ipfd);
}

int main(int argc, char** argv) {
  //   initServerConfig();
  if (argc >= 2) {
    char* configfile = NULL;
    configfile = argv[1];
    if (configfile) server.configfile = getAbsolutePath(configfile);
    loadServerConfig(server.configfile);
  } else {
    printf("ex: %s conf/server.conf\n", argv[0]);
    exit(-1);
  }
  initServer();
  bworkerInit();
  //   if (configfile) server.configfile = getAbsolutePath(configfile);
  //   loadServerConfig(configfile, options);

  //   // loadDataFromDisk();

  //   if (server.maxmemory > 0 && server.maxmemory < 1024 * 1024) {
  //     redisLog(REDIS_WARNING,
  //              "WARNING: You specified a maxmemory value that is less than
  //              1MB "
  //              "(current value is %llu bytes). Are you sure this is what
  //              you " "really want?", server.maxmemory);
  //   }
  //   server.el->stop = 0;
  //   while (!server.el->stop) {
  //     aeProcessEvents(eventLoop, AE_ALL_EVENTS);
  //   }
  for (;;) {
    processEvents();
  }
  return 0;
}