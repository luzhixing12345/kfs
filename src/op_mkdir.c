
#include "inode.h"
#include "logging.h"
#include "ops.h"

/** Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int op_mkdir(const char *path, mode_t mode) {
    mode = mode | S_IFDIR;
    DEBUG("mkdir %s with mode %o", path, mode);

    // find directory
    return 0;
}