
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

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
 * expected to reset the setuid and setgid bits.
 */
int op_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    DEBUG("write path %s with size %d offset %d", path, size, offset);

    int ret = 0;

    struct ext4_inode *inode;
    uint32_t inode_idx;
    if (inode_get_by_path(path, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }

    // check permissions
    if (inode_check_permission(inode, READ) && inode_check_permission(inode, WRITE)) {
        ERR("Permission denied");
        return -EACCES;
    }

    uint64_t start_lblock = offset / BLOCK_SIZE;
    uint64_t end_lblock = (offset + size - 1) / BLOCK_SIZE;
    uint32_t start_block_off = offset % BLOCK_SIZE;

    if (start_lblock == end_lblock) {
        // only one block to write
        ret = disk_write(BLOCKS2BYTES(inode_get_data_pblock(inode, start_lblock, NULL)) + start_block_off, size, (void *)buf);
    } else {
        ASSERT(0);
    }

    // uint64_t file_size = EXT4_INODE_SIZE(inode);
    EXT4_INODE_SET_SIZE(inode, size); // FIXME
    ICACHE_DIRTY(inode);
    DEBUG("write done");

    ASSERT(ret == size);
    return ret;
}