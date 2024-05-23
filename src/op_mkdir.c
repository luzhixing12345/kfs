
#include <stdint.h>
#include <string.h>
#include "dentry.h"
#include "ext4/ext4_basic.h"
#include "logging.h"
#include "bitmap.h"
#include "ops.h"

/** Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int op_mkdir(const char *path, mode_t mode) {
    mode = mode | S_IFDIR;
    DEBUG("mkdir %s with mode %o", path, mode);

    // first find the parent directory of this path
    uint32_t parent_idx = inode_get_parent_idx_by_path(path);
    if (parent_idx == 0) {
        // root directory
        return -ENOENT;
    }
    struct ext4_inode parent_inode;
    if (inode_get_by_number(parent_idx, &parent_inode) < 0) {
        DEBUG("fail to get parent inode %d", parent_idx);
        return -ENOENT;
    }

    // check if the parent inode has empty space for a new dentry
    uint64_t lblock; // for disk write back
    struct ext4_dir_entry_2 *de = dentry_last(&parent_inode, parent_idx, &lblock);
    if (de == NULL) {
        ERR("parent inode has no dentry");
        return -ENOENT;
    }
    char *dir_name = strrchr(path, '/') + 1;
    __le16 rec_len = ALIGN_TO_DENTRY(EXT4_DE_BASE_SIZE + strlen(dir_name));
    __le16 left_space = de->rec_len - DE_REAL_REC_LEN(de);
    INFO("dir_name %s, rec_len %u, last de left space %u", dir_name, rec_len, left_space);
    if (left_space < rec_len) {
        ERR("No space for new dentry");
        return -ENOSPC;
    }
    // find a valid inode_idx in GDT
    uint32_t dir_idx = bitmap_inode_find(parent_idx);
    if (dir_idx == 0) {
        ERR("No free inode");
        return -ENOSPC;
    }
    
    // find an empty data block
    uint64_t block_idx = bitmap_block_find(dir_idx);
    if (block_idx == 0) {
        ERR("No free block");
        return -ENOSPC;
    }

    INFO("new dentry %s, dir_idx %u, block_idx %u", dir_name, dir_idx, block_idx);
    INFO("start to create new dentry and new inode");

    // create new dentry
    if (dentry_create(de, dir_name, dir_idx) < 0) {
        ERR("fail to create dentry");
        return -ENOENT;
    }

    // dentry_write_back(uint64_t pblock, uint64_t block_id)

    return 0;
}