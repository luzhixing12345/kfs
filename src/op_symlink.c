
#include <string.h>
#include <sys/types.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_extents.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

static int inode_symlink_create(struct ext4_inode *inode, uint32_t inode_idx, const char *path) {
    // a small perf for symbolic links
    // if the link len <= 60, save it in the inode
    int link_len = strlen(path);
    if (link_len <= sizeof(inode->i_block)) {
        // <= 60 bytes: Link destination fits in inode
        memcpy(&inode->i_block, path, link_len);
    } else {
        // FIXME: create a new pblock for the symbolic link
        // TEST-CASE: [014]
        if (link_len > BLOCK_SIZE) {
            ERR("link len %u > BLOCK_SIZE %u", link_len, BLOCK_SIZE);
            return -EINVAL;
        }
        uint64_t pblock = bitmap_pblock_find(inode_idx, EXT4_INODE_PBLOCK_NUM);
        inode_init_pblock(inode, pblock);
        disk_write(BLOCKS2BYTES(pblock), link_len, (void *)path);
    }
    EXT4_INODE_SET_SIZE(inode, link_len);
    INFO("create symbolic inode %s", path);
    return 0;
}

// TODO: check if to exsists
// TODO: ln -s xx dir
int op_symlink(const char *from, const char *to) {
    DEBUG("create soft symlink from %s to %s", from, to);

    // symbolic link do not increase link count!
    uint32_t inode_idx;
    if ((inode_idx = bitmap_inode_find(0)) == 0) {
        ERR("No free inode");
        return -ENOSPC;
    }

    INFO("check ok");
    struct ext4_inode *inode;
    mode_t sym_mode = S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;
    inode_create(inode_idx, sym_mode, &inode);

    int err;
    if ((err = inode_symlink_create(inode, inode_idx, from)) < 0) {
        return err;
    }

    // create a new dentry in parent dir
    struct ext4_inode *to_dir_inode;
    uint32_t to_dir_inode_idx;
    if (inode_get_parent_by_path(to, &to_dir_inode, &to_dir_inode_idx) < 0) {
        DEBUG("fail to get inode %s", to);
        return -ENOENT;
    }

    struct ext4_dir_entry_2 *last_de = dentry_last(to_dir_inode, to_dir_inode_idx);
    char *name = strrchr(to, '/') + 1;
    if (dentry_has_enough_space(last_de, name) < 0) {
        ERR("No space for new dentry");
        return -ENOSPC;
    }
    struct ext4_dir_entry_2 *new_de = dentry_create(last_de, name, inode_idx, EXT4_FT_SYMLINK);
    ICACHE_SET_LAST_DE(inode, new_de);
    dcache_write_back();

    return 0;
}