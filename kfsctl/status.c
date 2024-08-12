
#include <stdio.h>
#include <unistd.h>

#include "cmd.h"
#include "ctl.h"

extern int kfsctl_fd;

int status_main(int argc, const char **argv) {
    if (ctl_init() < 0) {
        return -1;
    }

    printf("FileSystem status:\n");
    ctl_destroy();
    return 0;
}