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
  chLog(LOG_NOTICE, "%s", msg);
  if (sig == SIGINT) {
    printf("You insist... exiting now.\n");
    flashCacheDestroy(server.db->flashCache);
    exit(1);
  }
}
int setupSignalHandlers(void) {
  sighandler_t sig_ret;
  sig_ret = signal(SIGTERM, sigShutdownHandler);
  if (sig_ret == SIG_ERR) {
    chLog(LOG_ERROR, "[server] can't set SIGTERM handler");
    return EXIT_FAILURE;
  }
  signal(SIGINT, sigShutdownHandler);
  if (sig_ret == SIG_ERR) {
    chLog(LOG_ERROR, "[server] can't set SIGINT handler");
    return EXIT_FAILURE;
  }
  return 0;
}
void initServer(void) {
  server.pid = getpid();
  setupSignalHandlers();
  // /* Open the TCP listening socket for the user commands. */
  server.ipfd = tcpConnect(server.port);
  server.db = chmalloc(sizeof(struct kvDb));
  if (server.cache_mode == MODE_MEM_FIFO) {
    server.db->memCache = chmalloc(sizeof(struct hash));
    server.db->memCache->ht[0].size = 100;
    server.db->memCache->ht[0].sizemask = 100;
    server.db->memCache->ht[0].used = 0;
    server.db->memCache->ht[0].table =
        chcalloc(sizeof(struct hashEntry*) * 100);
    server.db->memCache->iterators = 0;
  } else if (server.cache_mode == MODE_MEM_LRU)
    server.db->memLRU = LRUCacheCreate(server.db);
  server.db->memQueue = QueueCreate(server.db);
  server.db->flashCache =
      flashCacheCreate(server.flashCache_dir, server.db, server.flashCache_size,
                       server.flashCache_file_size);
}
int main(int argc, char** argv) {
  //   initServerConfig();
  if (argc >= 2) {
    char* configfile = NULL;
    configfile = argv[1];
    //    if (configfile) server.configfile = getAbsolutePath(configfile);
    loadServerConfig(configfile);
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