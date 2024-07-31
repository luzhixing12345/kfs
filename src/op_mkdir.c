
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4_basic.h"
#include "ext4/ext4_inode.h"
#include "ext4/ext4_super.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

extern struct dcache *dcache;

/** Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int op_mkdir(const char *path, mode_t mode) {
    mode = mode | S_IFDIR;
    DEBUG("mkdir %s with mode %o", path, mode);

    // first find the parent directory of this path}
    uint32_t parent_idx;
    struct ext4_inode *parent_inode;
    if (inode_get_parent_by_path(path, &parent_inode, &parent_idx) < 0) {
        DEBUG("fail to get parent inode %d", parent_idx);
        return -ENOENT;
    }

    // check if disk has space for a new inode
    uint32_t dir_idx;
    uint64_t dir_pblock_idx;
    if ((dir_idx = bitmap_inode_find(parent_idx)) == 0) {
        ERR("No free inode");
        return -ENOSPC;
    }
    if ((dir_pblock_idx = bitmap_pblock_find(dir_idx, EXT4_INODE_PBLOCK_NUM)) == 0) {
        ERR("No free pblock");
        return -ENOSPC;
    }

    // check if the parent dentry has empty space for a new dentry
    struct ext4_dir_entry_2 *de = dentry_last(parent_inode, parent_idx);
    if (de == NULL) {
        ERR("parent inode has no dentry");
        return -ENOENT;
    }

    // create a new dentry, and write back to disk
    char *dir_name = strrchr(path, '/') + 1;
    uint64_t name_len = strlen(dir_name);
    if (dentry_has_enough_space(de, name_len) < 0) {
        // FIXME: try to find a new block for dir
        // TEST-CASE: [012]
        ERR("No space for new dentry");
        return -ENOSPC;
    }

    // create a new dentry in the parent directory
    struct ext4_dir_entry_2 *new_de = dentry_create(de, dir_name, dir_idx, inode_mode2type(mode));
    ICACHE_SET_LAST_DE(parent_inode, new_de);
    dcache_write_back();

    // set parent inode link count + 1 because ..
    parent_inode->i_links_count++;
    ICACHE_SET_DIRTY(parent_inode);

    // create a new inode
    struct ext4_inode *inode;
    inode_create(dir_idx, mode, &inode);
    inode_init_pblock(inode, dir_pblock_idx);

    // update inode and data bitmap
    bitmap_inode_set(dir_idx, 1);
    bitmap_pblock_set(dir_pblock_idx, EXT4_INODE_PBLOCK_NUM, 1);
    // just set bitmap and not write back to disk here for performance
    // write back bitmap to disk when fs is destory
    INFO("set inode and data bitmap");

    // update gdt
    gdt_update(dir_idx);

    // add . and .. for the new dentry
    dentry_init(parent_idx, dir_idx);
    INFO("add . and .. for the new dentry");
    disk_write_block(dir_pblock_idx, dcache->buf);
    INFO("write back . and .. for the new dentry to disk");
    return 0;
}