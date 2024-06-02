
#include <stdint.h>

#include "cache.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

extern struct ext4_super_block sb;

/** Change the permission bits of a file
 *
 * `fi` will always be NULL if the file is not currently open, but
 * may also be NULL if the file is open.
 */
int op_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
    DEBUG("chmod path %s with mode %o", path, mode);

    struct ext4_inode *inode;
    uint32_t inode_idx;
    if (fi && fi->fh) {
        inode_idx = fi->fh;
    } else {
        inode_idx = inode_get_idx_by_path(path);
    }

    if (inode_get_by_number(inode_idx, &inode) < 0) {
        INFO("fail to get inode %d", inode_idx);
        return -ENOENT;
    }

    // check permission
    if (inode_check_permission(inode, RDWR) < 0) {
        return -EACCES;
    }

    inode->i_mode = mode;

    // the inode do not need to be written back to disk for now
    // we just update it in cache(memory), and it will be written back to disk when:
    //
    // - the icache_entry is replaced by a new inode (see icache_lru_replace)
    // - fsync is called
    // - fs is destroyed(unmount)

    ICACHE_SET_DIRTY(inode);

    INFO("Finished chmod on inode %d", inode_idx);
    return 0;
}