
#include <stdint.h>
#include "logging.h"
#include "ops.h"
#include "ext4/ext4_dentry.h"
#include "dcache.h"
#include "inode.h"

extern struct dcache_entry root;

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 */
int op_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    DEBUG("create path %s with flags %o", path, fi->flags);
    struct dcache_entry *dc_entry = get_cached_inode_idx(path);
    uint32_t inode_idx = dc_entry ? dc_entry->inode_idx : root.inode_idx;
    DEBUG("Found inode_idx %d", inode_idx);

    // uint32_t pflags = 0;
    return 0;
}