
#include "ctl.h"

#include <assert.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

#include "bitmap.h"
#include "disk.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

#define BUFFER_SIZE     128
#define CACHE_SIGNATURE 0x12345678
#define CACHE_VERSION   1

const char *KFSCTL_FILENAME = "/.kfsctl";

struct CacheHeader {
    uint32_t signature;
    uint32_t version;
    uint32_t entries;
};

#define MAX_ENTRIES 5

struct CacheEntry {
    char filename[256];
    uint32_t inode_idx[MAX_ENTRIES];
};

int ctl_check(const char *path) {
    char *filename = strrchr(path, '/') + 1;
    if (filename == path + 1 || strcmp(filename, ".kfsctl") != 0) {
        return 0;
    }
    return 1;
}

int ctl_add_cache(const char *cache_path, uint32_t cache_ino) {
    uint32_t inode_idx;
    struct ext4_inode *inode;
    if (inode_get_by_path(KFSCTL_FILENAME, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode /.kfsctl");
        return -ENOENT;
    }

    struct CacheHeader header;
    op_read(KFSCTL_FILENAME, (char *)&header, sizeof(header), 0, NULL);
    if (header.signature != CACHE_SIGNATURE || header.version != CACHE_VERSION) {
        DEBUG("invalid cache header");
        header.signature = CACHE_SIGNATURE;
        header.version = CACHE_VERSION;
        header.entries = 0;
    }
    header.entries++;
    DEBUG("entries: %d", header.entries);
    
    return 0;
}

int ctl_add(const char *filename) {
    DEBUG("ctl add %s", filename);

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

    uint64_t pblock_idx = bitmap_pblock_find(inode_idx, EXT4_INODE_PBLOCK_NUM);
    inode_init_pblock(inode, pblock_idx);
    disk_write(BLOCKS2BYTES(pblock_idx), compress_size, compress_buffer);
    free(compress_buffer);
    DEBUG("write compressed buffer pblock[%lu] into disk", pblock_idx);

    ctl_add_cache(filename, new_inode_idx);

    return 0;
}

int ctl_log(char *buf) {
    struct ext4_inode *inode;
    if (inode_get_by_number(EXT4_KFSCTL_INO, &inode) < 0) {
        DEBUG("fail to get inode %d", EXT4_KFSCTL_INO);
        return -ENOENT;
    }
    return 0;
}

int ctl_status(char *buf) {
    int buf_cnt = 0;

    buf_cnt += sb_status(buf);

    uint64_t used_inode_num;
    uint64_t free_inode_num;
    bitmap_inode_count(&used_inode_num, &free_inode_num);
    buf_cnt +=
        sprintf(buf + buf_cnt, "  inode[free:total]\t [%lu/%lu]\n", used_inode_num, free_inode_num + used_inode_num);

    uint64_t used_pblock_num;
    uint64_t free_pblock_num;
    bitmap_pblock_count(&used_pblock_num, &free_pblock_num);
    buf_cnt += sprintf(
        buf + buf_cnt, "  pblock[free:total]\t [%lu/%lu]\n", used_pblock_num, free_pblock_num + used_pblock_num);

    return buf_cnt;
}

int ctl_read(char *buf, enum kfs_cmd cmd) {
    switch (cmd) {
        case CMD_STATUS:
            return ctl_status(buf);
        case CMD_LOG:
            return ctl_log(buf);
        default:;
    }
    ASSERT(0);
    return 0;
}

int ctl_write(const char *filename, enum kfs_cmd cmd) {
    switch (cmd) {
        case CMD_ADD:
            return ctl_add(filename);
        default:
            return -1;
    }
    ASSERT(0);
    return 0;
}
