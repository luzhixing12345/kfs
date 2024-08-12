/*
 * A trivial unit test to check linking without glib. A real test suite should
 * probably based off libvhost-user-glib instead.
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/unistd.h>
#include <unistd.h>

#include "libvhost-user.h"

static void panic(VuDev *dev, const char *err) {
    abort();
}

static void set_watch(VuDev *dev, int fd, int condition, vu_watch_cb cb, void *data) {
    abort();
}

static void remove_watch(VuDev *dev, int fd) {
    abort();
}

static const VuDevIface iface = {
    0,
};
// 初始化 Unix 域套接字
int init_socket(const char *socket_path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    unlink(socket_path);  // 确保地址没有被占用

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 1) < 0) {
        perror("listen");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

int main(int argc, const char *argv[]) {
    bool rc;
    uint16_t max_queues = 2;
    int socket = 0;
    char *socket_path = "/tmp/vhostqemu";
    socket = init_socket(socket_path);
    VuDev dev = {
        0,
    };

    rc = vu_init(&dev, max_queues, socket, panic, NULL, set_watch, remove_watch, &iface);
    assert(rc == true);
    vu_deinit(&dev);
    return 0;
}
