
#include <stdint.h>

#include "dcache.h"
#include "ext4/ext4_dentry.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

extern struct dcache_entry root;
extern struct ext4_super_block sb;
extern struct ext4_group_desc *gdt;

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
    DEBUG("create path %s with mode %o", path, mode);

    return 0;
}