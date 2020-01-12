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
  server.db = malloc(sizeof(struct kvDb));
  server.db->memCache = malloc(sizeof(struct hash));
  server.db->memCache->ht[0].table = malloc(server.db_size);
  server.db->memCache->ht[0].size = server.db_size;
  server.db->memCache->ht[0].sizemask =
      server.db_size / sizeof(struct hashEntry);
  server.db->memCache->ht[0].used = 0;
  int i = 0;
  // for (i = 0; i < 2; i++) {
  //   server.db->memCache->ht[i].table = NULL;
  //   server.db->memCache->ht[i].size = 0;
  //   server.db->memCache->ht[i].sizemask = 0;
  //   server.db->memCache->ht[i].used = 0;
  // }
  server.db->memCache->iterators = 0;
  server.db->memCache->rehashidx = -1;
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
  for (;;) {
  }
  return 0;
}