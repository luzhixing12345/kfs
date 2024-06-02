
#include <string.h>
#include <sys/types.h>
#include "bitmap.h"
#include "cache.h"
#include "dentry.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_dentry.h"
#include "ext4/ext4_inode.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

#define RENAME_NOREPLACE 0
#define RENAME_EXCHANGE  1

/** Rename a file
 *
 * *flags* may be `RENAME_EXCHANGE` or `RENAME_NOREPLACE`. If
 * RENAME_NOREPLACE is specified, the filesystem must not
 * overwrite *newname* if it exists and return an error
 * instead. If `RENAME_EXCHANGE` is specified, the filesystem
 * must atomically exchange the two files, i.e. both must
 * exist and neither may be deleted.
 */
int op_rename(const char *from, const char *to, unsigned int flags) {
    DEBUG("rename from %s to %s with flags %d", from, to, flags);

    struct ext4_inode *from_inode_dir, *to_inode_dir;
    uint32_t from_inode_dir_idx, to_inode_dir_idx;

    if (inode_get_parent_by_path(from, &from_inode_dir, &from_inode_dir_idx) < 0) {
        DEBUG("fail to get inode %s dir", from);
        return -ENOENT;
    }
    DEBUG("find from dir inode %d", from_inode_dir_idx);

    if (inode_get_parent_by_path(to, &to_inode_dir, &to_inode_dir_idx) < 0) {
        DEBUG("fail to get inode %s dir", to);
        return -ENOENT;
    }
    DEBUG("find to dir inode %d", to_inode_dir_idx);
    return 0;
}