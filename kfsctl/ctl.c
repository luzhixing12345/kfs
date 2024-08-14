
#include "ctl.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int kfsctl_fd = -1;

#define KFSCTL_PATH "/tmp/kfsctl.sock"
#define KFSCTL_PORT 7777

int ctl_init() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(KFSCTL_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    kfsctl_fd = client_socket;
    printf("connected to server with fd %d\n", kfsctl_fd);
    return 0;
}

int ctl_destroy() {
    if (kfsctl_fd >= 0) {
        close(kfsctl_fd);
    }
    return 0;
}

int ctl_cmd(enum kfs_cmd cmd, const char *filename) {
    assert(kfsctl_fd >= 0);

    struct Request req;
    req.cmd = cmd;

    if (filename) {
        strcpy(req.filename, filename);
    }

    if (write(kfsctl_fd, &req, sizeof(req)) < 0) {
        perror("write failed");
        return -1;
    }

    struct Response resp;
    if (read(kfsctl_fd, &resp, sizeof(resp)) < 0) {
        perror("read failed");
        return -1;
    }
    if (resp.need_print) {
        printf("%s", resp.msg);    
    }
    return 0;
}
