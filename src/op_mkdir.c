
#include <stdint.h>
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

    // first find the parent directory of this path
    uint32_t parent_idx = inode_get_parent_idx_by_path(path);
    struct ext4_inode parent_inode;
    if (inode_get_by_number(parent_idx, &parent_inode) < 0) {
        DEBUG("fail to get parent inode %d", parent_idx);
        return -ENOENT;
    }
    // check if the parent inode has empty space for a new dentry
    
    return 0;
}