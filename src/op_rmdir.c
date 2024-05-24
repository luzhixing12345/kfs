
#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "ops.h"

int op_rmdir(const char *path) {
    DEBUG("rmdir %s", path);

    // remove parent inode's dentry

    struct ext4_inode *inode;
    uint32_t inode_idx;
    if (inode_get_by_path(path, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }
    if (!S_ISDIR(inode->i_mode)) {
        return -EISDIR;
    }
    
    // set inode bitmap to 0
    bitmap_inode_set(inode_idx, 0);

    return 0;
}