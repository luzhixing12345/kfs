

#pragma once

#include <fcntl.h>
#include <stdint.h>

enum kfs_cmd { CMD_STATUS = 1, CMD_LOG, CMD_ADD };

struct Request {
    enum kfs_cmd cmd;
    char filename[256];
};

struct Response {
    int need_print;
    char msg[1024];
};


#define BUFFER_SIZE     128
#define CACHE_SIGNATURE 0x12345678
#define CACHE_VERSION   1

struct CacheHeader {
    uint32_t signature;
    uint32_t version;
    uint32_t entries;
};

#define MAX_ENTRIES 10

struct CacheEntry {
    char filename[256];
    uint32_t count;
    uint32_t inode_idx[MAX_ENTRIES];
};

void* ctl_init(void *arg);
int ctl_check(const char *path);
int ctl_read(char *buf, enum kfs_cmd cmd);
int ctl_write(const char *filename, enum kfs_cmd cmd);