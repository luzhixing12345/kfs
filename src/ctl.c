
#include "ctl.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int kfsctl_fd = -1;

const char *KFSCTL_PATH = ".kfsctl";
extern struct ext4_super_block sb;

int ctl_init() {
    kfsctl_fd = open(KFSCTL_PATH, O_RDWR);

    if (kfsctl_fd < 0) {
        perror("open");
        return -1;
    }
    
    return 0;
}

int ctl_check(uint32_t inode_idx) {
    return inode_idx == EXT4_USR_QUOTA_INO;
}

int ctl_status(char *buf) {
    int buf_cnt = 0;

    buf_cnt += sb_status(buf);    

    uint64_t used_inode_num;
    uint64_t free_inode_num;
    bitmap_inode_count(&used_inode_num, &free_inode_num);
    buf_cnt += sprintf(buf + buf_cnt, "  inode[free:total]\t [%lu/%lu]\n", used_inode_num, free_inode_num + used_inode_num);

    uint64_t used_pblock_num;
    uint64_t free_pblock_num;
    bitmap_pblock_count(&used_pblock_num, &free_pblock_num);
    buf_cnt += sprintf(buf + buf_cnt, "  pblock[free:total]\t [%lu/%lu]\n", used_pblock_num, free_pblock_num + used_pblock_num);

    return buf_cnt;
}

int ctl_destroy() {
    if (kfsctl_fd >= 0) {
        close(kfsctl_fd);
    }
    return 0;
}