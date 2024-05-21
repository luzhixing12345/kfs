
#include <stdint.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"
extern unsigned fuse_capable;
extern struct ext4_super_block sb;

/** Change the owner and group of a file
 *
 * `fi` will always be NULL if the file is not currently open, but
 * may also be NULL if the file is open.
 *
 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
 * expected to reset the setuid and setgid bits.
 */
int op_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
    DEBUG("chown path %s with uid %d gid %d", path, uid, gid);

    // check if FUSE_CAP_HANDLE_KILLPRIV is disabled
    if (!(fuse_capable & FUSE_CAP_HANDLE_KILLPRIV)) {
        INFO("FUSE_CAP_HANDLE_KILLPRIV is disabled");
        return -ENOSYS;
    }

    // FIXME: bug by "sudo chown root a.txt"

    // if gid < 0, operation not permitted
    if ((signed)gid < 0) {
        INFO("Permission denied");
        return -EPERM;
    }

    // if uid < 0, not a valid uid
    if ((signed)uid < 0) {
        INFO("invalid user id");
        return -EINVAL;
    }

    uint32_t inode_idx;

    if (fi && fi->fh) {
        inode_idx = fi->fh;
    } else {
        inode_idx = inode_get_idx_by_path(path);
    }

    if (inode_idx == 0) {
        return -ENOENT;
    }

    struct ext4_inode inode;
    uint64_t off = inode_get_offset(inode_idx);

    // read disk by inode number
    disk_read(off, MIN(EXT4_S_INODE_SIZE(sb), sizeof(struct ext4_inode)), &inode);

    if (inode_check_permission(&inode, RDWR) < 0) {
        INFO("Permission denied");
        return -EACCES;
    }

    // directly return if uid or gid is same as current
    if (uid == inode.i_uid && gid == inode.i_gid) {
        return 0;
    }

    inode.i_uid = uid;
    inode.i_gid = gid;
    disk_write(off, MIN(EXT4_S_INODE_SIZE(sb), sizeof(struct ext4_inode)), &inode);
    return 0;
}