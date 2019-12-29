#include "bworker.h"
void *bworkerRecvJobs(void *arg) {
  int cfd = (int)arg;
  chkangLog(LOG_NOTICE, "[%d] bworkerRecvJobs created", cfd);
  while (1) {
    tcpRecv(cfd);
    pthread_mutex_lock(&bworker_mutex[BPB_TCP_WORKER]);
    // 버퍼에 서버가 처리할 받은 데이터 저장.
    pthread_mutex_unlock(&bworker_mutex[BPB_TCP_WORKER]);
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
      if (cfd == MAX_NUM_CLIENT) {
        chkangLog(LOG_NOTICE, "[%d] connected client full", cfd);
        close(cfd);
        continue;
      }
      if (cfd < 0) {
        chkangLog(LOG_ERROR, "fail to call accept()");
        continue;
      }
      if (pthread_create(c_tid + num_client++, NULL, bworkerRecvJobs,
                         (void *)cfd) != 0) {
        chkangLog(LOG_ERROR, "Fatal: Can't initialize Background Jobs.");
        exit(-1);
      }
    }
  }
}
void bworkerInit(void) {
  pthread_attr_t attr;
  pthread_t thread;
  size_t stacksize;
  int i;

  /* Initialization of state vars and objects */
  for (i = 0; i < NUM_BWORKER; i++) {
    pthread_mutex_init(&bworker_mutex[i], NULL);
    pthread_cond_init(&bworker_cond[i], NULL);
    // bio_jobs[j] = listCreate();
    // bio_pending[j] = 0;
  }

  //   /* Set the stack size as by default it may be small in some system */
  pthread_attr_init(&attr);
  pthread_attr_getstacksize(&attr, &stacksize);
  if (!stacksize) stacksize = 1; /* The world is full of Solaris Fixes */
  while (stacksize < THREAD_STACK_SIZE) stacksize *= 2;
  pthread_attr_setstacksize(&attr, stacksize);

  for (i = 0; i < NUM_BWORKER; i++) {
    void *arg = (void *)(unsigned long)i;
    if (pthread_create(&thread, &attr, bworkerProcessBackgroundJobs, arg) !=
        0) {
      chkangLog(LOG_ERROR, "Fatal: Can't initialize Background Jobs.");
      exit(-1);
    }
    bworker_threads[i] = thread;
  }
}