
#include <stddef.h>
#include <string.h>
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

int op_release(const char *path, struct fuse_file_info *fi) {
    DEBUG("release %s", path);
    return 0;
}