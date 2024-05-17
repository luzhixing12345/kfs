
#include "ops.h"
#include "logging.h"

int op_access(const char *path, int mask) {
    DEBUG("access path %s with mask %o", path, mask);
    return 0;
}