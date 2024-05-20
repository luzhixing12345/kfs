
#include <stdint.h>
#include <sys/time.h>

#include "disk.h"
#include "ext4/ext4_super.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"
#include "super.h"

extern struct ext4_super_block sb;

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
    int ret;

    uint32_t inode_idx;
    if (fi && fi->fh) {
        inode_idx = fi->fh;
    } else {
        inode_idx = inode_get_idx_by_path(path);
    }

    ret = inode_get_by_number(inode_idx, &inode);
    if (ret < 0) {
        return ret;
    }

    ret = inode_check_permission(&inode);
    if (ret < 0) {
        return ret;
    }

    // update the access and modification times
    inode.i_atime = asec;
    inode.i_mtime = msec;
    INFO("update inode atime %d, mtime %d", inode.i_atime, inode.i_mtime);

    // write back the inode
    inode_set_by_number(inode_idx, &inode);

    DEBUG("utimens done");
    return 0;
}