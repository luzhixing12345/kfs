
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

/** Get file system statistics
 *
 * The 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 */
int op_statfs(const char *path, struct statvfs *stbuf) {
    DEBUG("statfs path %s", path);
    return 0;
}