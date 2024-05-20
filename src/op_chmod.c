
#include <stdint.h>

#include "disk.h"
#include "ext4/ext4_super.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"
#include "super.h"

extern struct ext4_super_block sb;

/** Change the permission bits of a file
 *
 * `fi` will always be NULL if the file is not currently open, but
 * may also be NULL if the file is open.
 */
int op_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
    DEBUG("chmod path %s with mode %o", path, mode);

    struct ext4_inode inode;
    uint32_t inode_idx;
    if (fi && fi->fh) {
        inode_idx = fi->fh;
    } else {
        inode_idx = inode_get_idx_by_path(path);
    }

    if (inode_idx == 0) {
        return -ENOENT;
    }
    inode_idx--; /* Inode 0 doesn't exist on disk */
    // read disk by inode number
    uint64_t off = inode_get_offset(inode_idx);
    disk_read(off, MIN(EXT4_S_INODE_SIZE(sb), sizeof(struct ext4_inode)), &inode);

    if (inode_check_permission(&inode, RDWR)) {
        return -EACCES;
    }

    inode.i_mode = mode;
    disk_write(off, sizeof(struct ext4_inode), &inode);
    INFO("Finished chmod on inode %d", inode_idx);
    return 0;
}