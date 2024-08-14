#include <asm-generic/errno-base.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "ctl.h"

int add_main(int argc, const char **argv) {
    if (ctl_init() < 0) {
        fprintf(stderr, "ctl init failed\n");
        return -1;
    }

    if (argc != 2) {
        fprintf(stderr, "usage: kfsctl add <filename>\n");
        return -1;
    }

    const char *filename = argv[1];
    
    // check if the file exists
    if (access(filename, F_OK) != 0) {
        printf("%s doesn't exist\n", filename);
        goto end;
    }

    if (ctl_cmd(CMD_ADD, filename) < 0) {
        printf("errno: %d\n", errno);
        assert(0);
    }

    printf("add %s success\n", filename);

end:
    ctl_destroy();
    return 0;
}