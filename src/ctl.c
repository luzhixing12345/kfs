
#include "ctl.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <zlib.h>

#include "bitmap.h"
#include "disk.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

const char *KFSCTL_FILENAME = "/.kfsctl";

#define KFSCTL_PATH "/tmp/kfsctl.sock"
#define KFSCTL_PORT 7777

#define MAX_EVENTS  10
#define BACKLOG     5

int ctl_add(struct Request *req, struct Response *resp);
int ctl_status(struct Request *req, struct Response *resp);
int ctl_log(struct Request *req, struct Response *resp);
int ctl_add(struct Request *req, struct Response *resp);

void *ctl_init(void *arg) {
    // create a socket and wait for client to connect
    struct sockaddr_in server_addr, client_addr;
    int server_socket, client_socket;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(KFSCTL_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 1) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    INFO("Server listening on port %d...", KFSCTL_PORT);
    while (1) {
        DEBUG("Waiting for connection...");
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Accept failed");
            ERR("Connection refused from kfsctl %s:%d", inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port)); exit(EXIT_FAILURE);
        }
        DEBUG("Connection accepted from kfsctl %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        struct Request req;
        if (read(client_socket, &req, sizeof(req)) != sizeof(req)) {
            perror("Read failed");
            exit(EXIT_FAILURE);
        }
        struct Response resp;
        DEBUG("cmd %d", req.cmd);
        switch (req.cmd) {
            case CMD_STATUS:
                ctl_status(&req, &resp);
                break;
            case CMD_LOG:
                ctl_log(&req, &resp);
                break;
            case CMD_ADD:
                ctl_add(&req, &resp);
                break;
            default:
                break;
        }

        if (write(client_socket, &resp, sizeof(resp)) != sizeof(resp)) {
            perror("Write failed");
            exit(EXIT_FAILURE);
        }
        close(client_socket);
        // break;
    }

    return 0;
}

// void *ctl_init(void *arg) {
//     int server_fd, new_socket, epoll_fd;
//     struct sockaddr_in address;
//     int addrlen = sizeof(address);
//     struct epoll_event event, events[MAX_EVENTS];

//     // 创建服务器套接字
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     // 绑定套接字到端口
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(KFSCTL_PORT);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("bind failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     // 监听端口
//     if (listen(server_fd, BACKLOG) < 0) {
//         perror("listen failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     // 创建 epoll 实例
//     if ((epoll_fd = epoll_create1(0)) < 0) {
//         perror("epoll_create1 failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     // 将服务器套接字添加到 epoll 监听列表中
//     event.events = EPOLLIN;  // 监听输入事件
//     event.data.fd = server_fd;
//     if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
//         perror("epoll_ctl failed");
//         close(server_fd);
//         close(epoll_fd);
//         exit(EXIT_FAILURE);
//     }

//     while (1) {
//         int num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
//         if (num_fds < 0) {
//             perror("epoll_wait failed");
//             close(server_fd);
//             close(epoll_fd);
//             exit(EXIT_FAILURE);
//         }

//         for (int i = 0; i < num_fds; i++) {
//             if (events[i].data.fd == server_fd) {
//                 // 处理新的连接请求
//                 new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
//                 if (new_socket < 0) {
//                     perror("accept failed");
//                     close(server_fd);
//                     close(epoll_fd);
//                     exit(EXIT_FAILURE);
//                 }

//                 // 将新连接的套接字添加到 epoll 监听列表中
//                 event.events = EPOLLIN;
//                 event.data.fd = new_socket;
//                 if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event) < 0) {
//                     perror("epoll_ctl failed");
//                     close(new_socket);
//                     close(server_fd);
//                     close(epoll_fd);
//                     exit(EXIT_FAILURE);
//                 }
//             } else {
//                 // 处理已连接套接字的输入事件
//                 struct Request req;
//                 struct Response resp;

//                 if (read(events[i].data.fd, &req, sizeof(req)) != sizeof(req)) {
//                     perror("read failed");
//                     close(events[i].data.fd);
//                     epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
//                     continue;
//                 }

//                 switch (req.cmd) {
//                     case CMD_STATUS:
//                         ctl_status(&req, &resp);
//                         break;
//                     case CMD_LOG:
//                         ctl_log(&req, &resp);
//                         break;
//                     case CMD_ADD:
//                         ctl_add(&req, &resp);
//                         break;
//                     default:
//                         break;
//                 }

//                 if (write(events[i].data.fd, &resp, sizeof(resp)) != sizeof(resp)) {
//                     perror("write failed");
//                     close(events[i].data.fd);
//                     epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
//                     continue;
//                 }

//                 close(events[i].data.fd);
//                 epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
//             }
//         }
//     }

//     close(server_fd);
//     close(epoll_fd);
//     return NULL;
// }

int ctl_check(const char *path) {
    char *filename = strrchr(path, '/') + 1;
    if (filename == path + 1 || strcmp(filename, ".kfsctl") != 0) {
        return 0;
    }
    return 1;
}

int ctl_add_cache(const char *cache_path, uint32_t cache_ino) {
    DEBUG("add cache %s %d", cache_path, cache_ino);
    uint32_t inode_idx;
    struct ext4_inode *inode;
    if (inode_get_by_path(KFSCTL_FILENAME, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode /.kfsctl");
        return -ENOENT;
    }

    struct CacheHeader header;
    if (op_read(KFSCTL_FILENAME, (char *)&header, sizeof(header), 0, NULL) < 0) {
        ERR("fail to read /.kfsctl");
        return -EIO;
    }
    if (header.signature != CACHE_SIGNATURE || header.version != CACHE_VERSION) {
        ERR("invalid cache header");
        return -EINVAL;
    }
    DEBUG("entries: %d", header.entries);

    size_t cache_size = sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header.entries;
    char *buffer = malloc(cache_size);
    if (header.entries != 0) {
        if (op_read(KFSCTL_FILENAME, buffer, cache_size, 0, NULL) < 0) {
            ERR("fail to read /.kfsctl");
            free(buffer);
            return -EIO;
        }
        struct CacheEntry *entry = (struct CacheEntry *)(buffer + sizeof(struct CacheHeader));
        bool found = false;
        for (int i = 0; i < header.entries; i++) {
            DEBUG("finding in kfsctl file cache [%s]", entry[i].filename);
            if (strcmp(cache_path, entry[i].filename) == 0) {
                if (entry[i].count < MAX_ENTRIES) {
                    entry[i].inode_idx[entry[i].count] = cache_ino;
                    entry[i].count++;
                    found = true;
                    break;
                } else {
                    ERR("cache %s is full", cache_path);
                    free(buffer);
                    return -ENOSPC;
                }
            }
        }
        if (!found) {
            INFO("cache %s not found in /.kfsctl", cache_path);
            header.entries++;
            buffer = realloc(buffer, sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header.entries);
            entry = (struct CacheEntry *)(buffer + sizeof(struct CacheHeader));
            strcpy(entry[header.entries - 1].filename, cache_path);
            entry[header.entries - 1].count = 1;
            entry[header.entries - 1].inode_idx[0] = cache_ino;
            cache_size = sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header.entries;
        }
    } else {
        DEBUG("init kfsctl file cache [%s]", cache_path);
        header.entries = 1;
        cache_size = sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header.entries;
        buffer = realloc(buffer, cache_size);

        memcpy(buffer, &header, sizeof(struct CacheHeader));
        struct CacheEntry *entry = (struct CacheEntry *)(buffer + sizeof(struct CacheHeader));
        strcpy(entry[0].filename, cache_path);
        entry[0].count = 1;
        entry[0].inode_idx[0] = cache_ino;
    }

    if (op_write(KFSCTL_FILENAME, buffer, cache_size, 0, NULL) < 0) {
        ERR("fail to write /.kfsctl");
        free(buffer);
        return -EIO;
    }

    return 0;
}

int ctl_add(struct Request *req, struct Response *resp) {
    const char *filename = req->filename;
    DEBUG("ctl add %s", filename);
    resp->need_print = 0;

    uint32_t inode_idx;
    struct ext4_inode *inode;
    if (inode_get_by_path(filename, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", filename);
        return -ENOENT;
    }

    uint64_t inode_size = EXT4_INODE_GET_SIZE(inode);
    char *buffer = malloc(inode_size);
    if (op_read(filename, buffer, inode_size, 0, NULL) < 0) {
        free(buffer);
        return -EIO;
    }
    DEBUG("finish read: %lu bytes", inode_size);
    DEBUG("file content: %s", buffer);
    unsigned char *compress_buffer;
    uint64_t compress_size;
    if (inode_size <= BUFFER_SIZE) {
        compress_buffer = malloc(sizeof(char) * BUFFER_SIZE);
        compress_size = BUFFER_SIZE;
    } else {
        compress_buffer = malloc(sizeof(char) * inode_size);
        compress_size = inode_size;
    }
    compress(compress_buffer, &compress_size, (uint8_t *)buffer, inode_size);
    DEBUG("finish compress, size: %lu -> %lu", inode_size, compress_size);
    free(buffer);

    // check if the file already exists
    uint32_t new_inode_idx;
    if ((new_inode_idx = bitmap_inode_find(EXT4_ROOT_INO)) == 0) {
        ERR("No space for new inode");
        return -ENOSPC;
    }
    struct ext4_inode *new_inode;
    inode_create(new_inode_idx, 0444, &new_inode);
    bitmap_inode_set(new_inode_idx, 1);

    uint64_t pblock_idx = bitmap_pblock_find(new_inode_idx, EXT4_INODE_PBLOCK_NUM);
    inode_init_pblock(new_inode, pblock_idx);
    disk_write(BLOCKS2BYTES(pblock_idx), compress_size, compress_buffer);
    free(compress_buffer);
    DEBUG("write compressed buffer pblock[%lu] into disk", pblock_idx);

    return ctl_add_cache(filename, new_inode_idx);
}

int ctl_log(struct Request *req, struct Response *resp) {
    DEBUG("ctl log");
    int buf_cnt = 0;
    resp->need_print = 1;

    char *buffer = malloc(sizeof(struct CacheHeader));
    if (op_read(KFSCTL_FILENAME, buffer, sizeof(struct CacheHeader), 0, NULL) < 0) {
        ERR("fail to read /.kfsctl");
        buf_cnt += sprintf(resp->msg + buf_cnt, "fail to read /.kfsctl\n");
        goto end;
    }
    struct CacheHeader *header = (struct CacheHeader *)buffer;
    if (header->signature != CACHE_SIGNATURE || header->version != CACHE_VERSION) {
        ERR("invalid /.kfsctl");
        buf_cnt += sprintf(resp->msg + buf_cnt, "invalid /.kfsctl signature or version\n");
        goto end;
    }

    if (header->entries == 0) {
        buf_cnt += sprintf(resp->msg + buf_cnt, "no entries in /.kfsctl\n");
    } else {
        buf_cnt += sprintf(resp->msg + buf_cnt, "%u entries found\n", header->entries);
        buffer = realloc(buffer, sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header->entries);
        if (op_read(KFSCTL_FILENAME,
                    buffer,
                    sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header->entries,
                    0,
                    NULL) < 0) {
            ERR("fail to read /.kfsctl");
            goto end;
        }
        struct CacheEntry *entry = (struct CacheEntry *)(buffer + sizeof(struct CacheHeader));
        for (int i = 0; i < header->entries; i++) {
            buf_cnt += sprintf(resp->msg + buf_cnt, "%s\n", entry[i].filename);
            DEBUG("filename: %s", entry[i].filename);
            for (int j = 0; j < entry[i].count; j++) {
                buf_cnt += sprintf(resp->msg + buf_cnt, "  [%2d] inode[%u]\n", j, entry[i].inode_idx[j]);
                DEBUG("  [%2d] inode[%u]", j, entry[i].inode_idx[j]);
            }
        }
    }

end:
    free(buffer);
    return buf_cnt;
}

int ctl_status(struct Request *req, struct Response *resp) {
    int buf_cnt = 0;
    resp->need_print = 1;

    buf_cnt += sb_status(resp->msg);

    uint64_t used_inode_num;
    uint64_t free_inode_num;
    bitmap_inode_count(&used_inode_num, &free_inode_num);
    buf_cnt += sprintf(
        resp->msg + buf_cnt, "  inode[free:total]\t [%lu/%lu]\n", used_inode_num, free_inode_num + used_inode_num);

    uint64_t used_pblock_num;
    uint64_t free_pblock_num;
    bitmap_pblock_count(&used_pblock_num, &free_pblock_num);
    buf_cnt += sprintf(
        resp->msg + buf_cnt, "  pblock[free:total]\t [%lu/%lu]\n", used_pblock_num, free_pblock_num + used_pblock_num);

    return buf_cnt;
}