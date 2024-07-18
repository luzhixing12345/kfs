
#include <string.h>
#include <sys/types.h>

#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

static int inode_symlink_create(struct ext4_inode *inode, const char *path) {
    // a small perf for symbolic links
    // if the link len <= 60, save it in the inode
    int link_len = strlen(path);
    uint64_t pblock;
    if (link_len <= EXT4_N_BLOCKS * sizeof(inode->i_block)) {
        memcpy(inode->i_block, path, link_len);
    } else {
        if (link_len >= EXT4_BLOCK_SIZE(sb)) {
            ERR("link too long");
            return -ENAMETOOLONG;
        }
        pblock = inode_get_data_pblock(inode, 0, NULL);
        disk_write(pblock, link_len, (void *)path);
        EXT4_INODE_SET_BLOCKS(inode, 1);
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
    uint64_t pblock;
    if (bitmap_find_space(0, &inode_idx, &pblock) < 0) {
        ERR("No free inode");
        return -ENOSPC;
    }

    INFO("check ok");
    struct ext4_inode *inode;
    mode_t sym_mode = S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;
    inode_create(inode_idx, sym_mode, pblock, &inode);

    int err;
    if ((err = inode_symlink_create(inode, from)) < 0) {
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