
#include <stdint.h>

#include "disk.h"
#include "ext4/ext4_super.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"
#include "super.h"

/** Change the owner and group of a file
 *
 * `fi` will always be NULL if the file is not currently open, but
 * may also be NULL if the file is open.
 *
 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
 * expected to reset the setuid and setgid bits.
 */
int op_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
    DEBUG("chown path %s with uid %d gid %d", path, uid, gid);

    // check if FUSE_CAP_HANDLE_KILLPRIV is enabled
    
    return 0;
}