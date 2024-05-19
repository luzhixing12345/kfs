
#include "ops.h"
#include "logging.h"

int op_flush(const char *path, struct fuse_file_info *fi) {
    DEBUG("flush path %s", path);
    return 0;
}