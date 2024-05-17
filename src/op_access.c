
#include "ops.h"
#include "logging.h"

int op_access(const char *path, int mask) {
    DEBUG("access path %s with mask %o", path, mask);
    // check if has permissions
    struct fuse_context *cntx=fuse_get_context();
    mode_t umask_val = cntx->umask;
    DEBUG("umask %o", umask_val);
    // cache access
    return 0;
}