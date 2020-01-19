#include "bworker.h"
pthread_t bworker_threads[MAX_BWORKER];
pthread_mutex_t bworker_mutex;
sem_t *sem_bworker;

void *bworkerRecvJobs(void *arg) {
  int id = (int)arg;
  struct kvClient *c = server.clients[id];
  chLog(LOG_NOTICE, "[BWK] %d: bworkerRecvJobs created", id);
  while (1) {
    int rst = tcpRecv(c);
    if (rst == 0) {
      chLog(LOG_NOTICE, "[BWK] %d: Client closed connection", id);
      chfree(c->querybuf);
      chfree(c);
      return NULL;
    }
    parsingMessage(c);
    processCMD(c);
    int i;
    for (i = 0; i < 2; i++) chfree(c->argv[i]);
    tcpSend(c);
  }
}
void *bworkerProcessBackgroundJobs(void *arg) {
  unsigned long type = (unsigned long)arg;
  pthread_t c_tid[MAX_NUM_CLIENT] = {
      0,
  };
  int num_client = 0;

  while (1) {
    if (type == BPB_TCP_WORKER) {
      int cfd = tcpAccept(server.ipfd);
      // Client는 최대 MAX_NUM_CLIENT 만큼 까지만 수용
      if (num_client == MAX_NUM_CLIENT) {
        chLog(LOG_NOTICE, "[BWK] %d: connected client full", cfd);
        close(cfd);
        continue;
      }
      if (cfd < 0) {
        chLog(LOG_ERROR, "fail to call accept()");
        continue;
      }
      struct kvClient *c = chmalloc(sizeof(struct kvClient));
      c->fd = cfd;
      c->id = server.num_connected_client;
      c->querybuf = chmalloc(sizeof(struct msg) + KV_IOBUF_LEN);
      c->db = &server.db[0];
      server.clients[server.num_connected_client++] = c;
      if (pthread_create(c_tid + num_client, NULL, bworkerRecvJobs,
                         (void *)c->id) != 0) {
        chLog(LOG_ERROR, "[BWK] Fatal: Can't initialize Background Jobs.");
        exit(-1);
      }
      bworker_threads[NUM_BWORKER + c->id] = c_tid + num_client;
      num_client++;
    } else if (type == BPB_TIERING_FLASH) {
      sem_wait(sem_bworker);
      struct hashEntry *he = QueueDequeue(server.db->memQueue);
      char *evict_key = he->key;
      struct kvObject *valueObj = he->val;
      pthread_mutex_lock(&bworker_mutex);
      flashCacheInsert(server.db->flashCache, evict_key, valueObj->ptr->buf,
                       valueObj->ptr->len);
      LRUCacheErase(server.db->memLRU, evict_key);
      chfree(he->key);
      chfree(he->val);
      chfree(he);
      pthread_mutex_unlock(&bworker_mutex);
    }
  }
}
void bworkerInit(void) {
  pthread_t thread;
  pthread_mutex_init(&bworker_mutex, NULL);
#ifdef __APPLE__
  sem_bworker = sem_open("sem_bworker", O_CREAT, S_IRWXU, 0);
  if (sem_bworker == SEM_FAILED) {
    chLog(LOG_ERROR, "unable to create Background semaphore");
    sem_unlink(sem_bworker);
    exit(-1);
  }
#else
  int ret = sem_init(sem_bworker, 0, 0);
  if (ret == -1) {
    chLog(LOG_ERROR, "Background semaphore init fail");
    exit(-1);
  }
#endif

  int i;
  for (i = 0; i < NUM_BWORKER; i++) {
    void *arg = (void *)(unsigned long)i;
    if (pthread_create(&thread, NULL, bworkerProcessBackgroundJobs, arg) != 0) {
      chLog(LOG_ERROR, "[BWK] Fatal: Can't initialize Background Jobs.");
      exit(-1);
    }
    bworker_threads[i] = thread;
  }
}