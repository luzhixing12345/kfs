
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

#include "bitmap.h"
#include "cache.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

bool enable_ctl = 0;

struct CacheHeader {
    uint32_t signature;
    uint32_t version;
    uint32_t entries;
};

#define MAX_ENTRIES 5

struct CacheEntry {
    char filename[256];
    uint32_t entries;
    unsigned char sha1[MAX_ENTRIES][SHA_DIGEST_LENGTH];
};

int ctl_check(const char *path) {
    char *filename = strrchr(path, '/') + 1;
    if (filename == path + 1 || strcmp(filename, ".kfsctl") != 0) {
        return 0;
    }
    return 1;
}

int ctl_add(const char *filename) {
    DEBUG("add %s", filename);

    // check if the file already exists
    uint32_t inode_idx;
    if ((inode_idx = bitmap_inode_find(EXT4_ROOT_INO)) == 0) {
        ERR("No space for new inode");
        return -ENOSPC;
    }
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

// int ctl_read(char *buf, enum kfs_cmd cmd) {
//     switch (cmd) {
//         case CMD_STATUS:
//             return ctl_status(buf);
//         case CMD_LOG:
//             return ctl_log(buf);
//         default:;
//     }
//     assert(0);
//     return 0;
// }

// int ctl_write(const char *filename, enum kfs_cmd cmd) {
//     switch (cmd) {
//         case CMD_ADD:
//             return ctl_add(filename);
//         default:
//             return -1;
//     }
//     assert(0);
//     return 0;
// }
