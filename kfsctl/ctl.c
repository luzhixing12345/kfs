
#include "ctl.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int kfsctl_fd = -1;

const char *KFSCTL_PATH = ".kfsctl";

int ctl_init() {
    kfsctl_fd = open(KFSCTL_PATH, O_RDWR);
    if (kfsctl_fd < 0) {
        printf("not kfs mounted\n");
        return -1;
    }
    return 0;
}

int ctl_destroy() {
    if (kfsctl_fd >= 0) {
        close(kfsctl_fd);
    }
    return 0;
}



int ctl_cmd(enum kfs_cmd cmd, const char *filename) {
    static char buf[1024];

    if (filename) {
        return pwrite(kfsctl_fd, filename, strlen(filename), CTL_OFFSET_CONSTANT * cmd);
    } else {
        int ret = pread(kfsctl_fd, buf, sizeof(buf), CTL_OFFSET_CONSTANT * cmd);
        buf[ret] = '\0';
        printf("%s\n", buf);
        return ret;
    }
}
