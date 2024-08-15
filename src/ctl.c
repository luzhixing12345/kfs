
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
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

const char *KFSCTL_FILENAME = "/.kfsctl";

#define KFSCTL_PATH "/tmp/kfsctl.sock"
#define KFSCTL_PORT 7777

int ctl_add(struct Request *req, struct Response *resp);
int ctl_status(struct Request *req, struct Response *resp);
int ctl_log(struct Request *req, struct Response *resp);
int ctl_add(struct Request *req, struct Response *resp);
int ctl_restore(struct Request *req, struct Response *resp);

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
            ERR("Connection refused from kfsctl %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            exit(EXIT_FAILURE);
        }
        DEBUG("Connection accepted from kfsctl %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        struct Request req;
        if (read(client_socket, &req, sizeof(req)) != sizeof(req)) {
            perror("Read failed");
            exit(EXIT_FAILURE);
        }
        struct Response resp;
        DEBUG("kfs cmd %d", req.cmd);
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
            case CMD_RESTORE:
                ctl_restore(&req, &resp);
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
                    clock_gettime(CLOCK_REALTIME, &entry[i].timestamp[entry[i].count]);
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
            clock_gettime(CLOCK_REALTIME, &entry[header.entries - 1].timestamp[0]);
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
        clock_gettime(CLOCK_REALTIME, &entry[0].timestamp[0]);
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
    // DEBUG("file content: %s", buffer);
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
    EXT4_INODE_SET_SIZE(new_inode, compress_size);
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
        header = (struct CacheHeader *)buffer;
        if (op_read(KFSCTL_FILENAME,
                    buffer,
                    sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header->entries,
                    0,
                    NULL) < 0) {
            ERR("fail to read /.kfsctl");
            goto end;
        }
        struct CacheEntry *entry = (struct CacheEntry *)(buffer + sizeof(struct CacheHeader));
        char time_string[100];
        time_t seconds;
        for (int i = 0; i < header->entries; i++) {
            buf_cnt += sprintf(resp->msg + buf_cnt, "%s\n", entry[i].filename);
            DEBUG("filename: %s", entry[i].filename);
            for (int j = 0; j < entry[i].count; j++) {
                seconds = (time_t)entry[i].timestamp[j].tv_sec;
                struct tm *local_time = localtime(&seconds);
                strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);
                buf_cnt += sprintf(resp->msg + buf_cnt, "  [%2d] %s\n", j, time_string);
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

    return 0;
}

int ctl_restore_cache(const char *filename, uint64_t inode_idx) {
    DEBUG("restore cache %s[%lu]", filename, inode_idx);
    struct ext4_inode *inode;
    if (inode_get_by_number(inode_idx, &inode) < 0) {
        ERR("fail to get inode[%lu]", inode_idx);
        return -1;
    }

    uint64_t inode_size = EXT4_INODE_GET_SIZE(inode);
    unsigned char *buffer = malloc(inode_size);
    struct fuse_file_info fi;
    fi.fh = inode_idx;
    if (op_read("", (char *)buffer, inode_size, 0, &fi) < 0) {
        free(buffer);
        return -EIO;
    }
    DEBUG("finish read: %lu bytes", inode_size);
    DEBUG("uncompress to file content");

    unsigned long buffer_size = BUFFER_SIZE;
    unsigned long temp_len = buffer_size;
    unsigned char *uncompress_buffer = (unsigned char *)malloc(sizeof(char) * (buffer_size + 1));
    uncompress(uncompress_buffer, &temp_len, buffer, inode_size);
    
    while (temp_len >= buffer_size) {
        DEBUG("oversize");
        buffer_size *= 2;
        uncompress_buffer = (unsigned char *)realloc(uncompress_buffer, sizeof(char) * buffer_size + 1);
        uncompress(uncompress_buffer, &temp_len, buffer, inode_size);
        uncompress_buffer[temp_len] = '\0';
    }

    DEBUG("finish uncompress: %lu bytes", temp_len);
    DEBUG("file content: %s", uncompress_buffer);

    struct stat st;
    if (op_getattr(filename, &st, NULL) < 0) {
        op_create(filename, 0755 | S_IFREG, NULL);
    }

    op_write(filename, (char *)uncompress_buffer, temp_len, 0, NULL);
    free(buffer);
    free(uncompress_buffer);
    return 0;
}

int ctl_restore(struct Request *req, struct Response *resp) {
    DEBUG("ctl restore");

    resp->need_print = 1;
    int buf_cnt = 0;

    ASSERT(req->data >= 0);
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
        buf_cnt += sprintf(resp->msg + buf_cnt, "no entries\n");
    } else {
        buffer = realloc(buffer, sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header->entries);
        header = (struct CacheHeader *)buffer;
        if (op_read(KFSCTL_FILENAME,
                    buffer,
                    sizeof(struct CacheHeader) + sizeof(struct CacheEntry) * header->entries,
                    0,
                    NULL) < 0) {
            ERR("fail to read /.kfsctl");
            goto end;
        }
        struct CacheEntry *entry = (struct CacheEntry *)(buffer + sizeof(struct CacheHeader));
        // char time_string[100];
        // time_t seconds;
        int found = 0;
        for (int i = 0; i < header->entries; i++) {
            if (strcmp(req->filename, entry[i].filename) == 0) {
                int version;
                found = 1;
                if (req->data >= entry[i].count || req->data == INT_MAX) {
                    buf_cnt += sprintf(resp->msg + buf_cnt, "restore to lastest version\n");
                    version = entry[i].count - 1;
                } else {
                    buf_cnt += sprintf(resp->msg + buf_cnt, "restore to version %d\n", req->data);
                    version = req->data;
                }

                if (ctl_restore_cache(entry[i].filename, entry[i].inode_idx[version]) < 0) {
                    ERR("fail to restore cache");
                    buf_cnt += sprintf(resp->msg + buf_cnt, "fail to restore cache\n");
                    goto end;
                }
                break;
            }
        }
        if (found == 0) {
            buf_cnt += sprintf(resp->msg + buf_cnt, "file [%s] not found\n", req->filename);
        }
    }

end:
    free(buffer);
    return 0;
}