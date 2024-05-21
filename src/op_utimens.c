
#include <stdint.h>
#include <sys/time.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

/**
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * This supersedes the old utime() interface.  New applications
 * should use this.
 *
 * `fi` will always be NULL if the file is not currently open, but
 * may also be NULL if the file is open.
 *
 * See the utimensat(2) man page for details.
 */
int op_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    DEBUG("utimens path %s", path);

    time_t asec = tv[0].tv_sec;
    time_t msec = tv[1].tv_sec;

    struct timeval now;
    gettimeofday(&now, NULL);
    if (asec == 0) {
        asec = now.tv_sec;
    }
    if (msec == 0) {
        msec = now.tv_sec;
    }

    struct ext4_inode inode;

    uint32_t inode_idx;
    if (fi && fi->fh) {
        inode_idx = fi->fh;
    } else {
        inode_idx = inode_get_idx_by_path(path);
    }

    // read disk by inode number
    if (inode_idx == 0) {
        return -ENOENT;
    }
    inode_idx--; /* Inode 0 doesn't exist on disk */

    uint64_t off = inode_get_offset(inode_idx);
    disk_read(off, MIN(EXT4_S_INODE_SIZE(sb), sizeof(struct ext4_inode)), &inode);

    if (inode_check_permission(&inode, WR_ONLY)) {
        return -EPERM;
    }

    // update the access and modification times
    inode.i_atime = asec;
    inode.i_mtime = msec;

    // write back the inode
    disk_write(off, MIN(EXT4_S_INODE_SIZE(sb), sizeof(struct ext4_inode)), &inode);

    DEBUG("utimens done");
    return 0;
}