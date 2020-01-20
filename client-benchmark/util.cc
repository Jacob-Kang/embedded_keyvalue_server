#include "util.h"

void loadServerConfig(char *filename) {
  /* Load the file content */
  char buf[1024];
  char *argv[10];
  int argc = 0;
  if (filename) {
    FILE *fp;
    if ((fp = fopen(filename, "r")) == NULL) {
      printf("Fatal error, can't open config file '%s'", filename);
      exit(1);
    }
    while (fgets(buf, 1024, fp) != NULL) {
      if (buf[0] == '#' || buf[0] == '\0' || buf[0] == '\n') continue;
      argv[argc] = strtok(buf, " ");
      while (argv[argc] != NULL) {
        argv[++argc] = strtok(NULL, " ");  // 다음 문자열을 잘라서 포인터를 반환
      }
      if (!strcasecmp(argv[0], "port") && argc == 2) {
        client.port = atoi(argv[1]);
      } else if (!strcasecmp(argv[0], "host") && argc == 2) {
        client.ip = std::string(argv[1]);
      }
      argc = 0;
    }
    if (fp != stdin) fclose(fp);
  }
}