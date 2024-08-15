
#include <errno.h>
#include <stdint.h>
#include <pthread.h>

enum kfs_cmd { CMD_STATUS = 1, CMD_LOG, CMD_ADD, CMD_RESTORE };

struct Request {
    enum kfs_cmd cmd;
    char filename[256];
    int data;
};

struct Response {
    int need_print;
    char msg[1024];
};

int ctl_init();
void * ctl_init_thread(void *arg);
int ctl_destroy();

int ctl_cmd(enum kfs_cmd cmd, const char *filename, int data);
