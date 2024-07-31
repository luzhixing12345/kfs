
#include <stdio.h>
#include <unistd.h>
#include "cmd.h"
#include "ctl.h"

extern int kfsctl_fd;

int status_main(int argc, const char **argv) {

    if (ctl_init() < 0) {
        fprintf(stderr, "ctl init failed\n");
        return -1;
    }

    char buf[1024];
    // read file content
    int ret = 0;
    if ((ret = read(kfsctl_fd, buf, sizeof(buf))) < 0) {
        perror("read");
        return -1;
    }
    buf[ret] = '\0';
    printf("%s\n", buf);

    return 0;
}