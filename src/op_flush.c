
#include <string.h>
#include <sys/types.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

extern struct dcache *dcache;

int op_flush(const char *path, struct fuse_file_info *fi) {
    DEBUG("flush path %s", path);

    struct ext4_inode *inode;
    uint32_t inode_idx;
    if (inode_get_by_path(path, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }

    if (ICACHE_IS_DIRTY(inode)) {
        INFO("write back dirty inode %d", inode_idx);
        icache_write_back((struct icache_entry *)inode);
    }

    // flush data block to disk
    if (dcache->inode_idx == inode_idx) {
        dcache_write_back();
    }

    DEBUG("finish flush");
    return 0;
}