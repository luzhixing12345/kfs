#include <asm-generic/errno-base.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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
    // int fd = open(".kfsctl", O_RDONLY);
    // if (fd < 0) {
    //     perror("open .kfsctl failed");
    //     goto end;
    // }
    // char path[256];
    // pread(fd, path, sizeof(path) - 1, 4096);
    // // printf("fs_path: %s\n", path);
    // char result[1024];
    // const char *last_slash = strrchr(path, '/');
    // size_t length = last_slash - path;
    // strncpy(result, path, length);
    // result[length] = '\0';
    // snprintf(result + length, sizeof(result) - length, "/%s", filename);
    // close(fd);

    if (ctl_cmd(CMD_ADD, filename, -1) < 0) {
        printf("errno: %d\n", errno);
        assert(0);
    }

    printf("add %s success\n", filename);

end:
    ctl_destroy();
    return 0;
}