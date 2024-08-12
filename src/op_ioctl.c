#include <stddef.h>
#include <sys/types.h>

#include "bitmap.h"
#include "cache.h"
#include "ctl.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_inode.h"
#include "extents.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

int op_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data) {
    DEBUG("ioctl %s with cmd %d", path, cmd);
    return 0;
}