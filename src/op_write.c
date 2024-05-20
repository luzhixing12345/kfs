
#include "ops.h"
#include "logging.h"

int op_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    DEBUG("write path %s with size %d offset %d", path, size, offset);
    return 0;
}