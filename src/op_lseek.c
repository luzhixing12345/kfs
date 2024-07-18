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

off_t op_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {
    DEBUG("lseek path %s", path);

    return 0;
}