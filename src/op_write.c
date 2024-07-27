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
    if (inode_check_permission(inode, READ) < 0 || inode_check_permission(inode, WRITE) < 0) {
        ERR("Permission denied");
        return -EACCES;
    }

    uint64_t start_lblock = offset / BLOCK_SIZE;
    uint64_t end_lblock = (offset + size - 1) / BLOCK_SIZE;
    uint32_t start_block_off = offset % BLOCK_SIZE;

    uint32_t extend_len = 0;

    if (start_lblock == end_lblock) {
        // only one block to write
        ret = disk_write(
            BLOCKS2BYTES(inode_get_data_pblock(inode, start_lblock, &extend_len)) + start_block_off, size, (void *)buf);
    } else {
        // FIXME : 读出数据与原始数据不一致
        for (uint64_t lblock = start_lblock; lblock <= end_lblock; ++lblock) {
            uint32_t block_off = (lblock == start_lblock) ? start_block_off : 0;

            inode->i_flags=0;

            size_t block_size =
                (lblock == end_lblock) ? (offset + size - (lblock * BLOCK_SIZE)) : (BLOCK_SIZE - block_off);

            assert(block_size <= BLOCK_SIZE);

            ret = disk_write(
                BLOCKS2BYTES(inode_get_data_pblock(inode, lblock, &extend_len)) + block_off, block_size, (void *)buf);
            if (ret != block_size) {
                return -EIO;
            }
            buf += block_size;
        }
    }

    EXT4_INODE_SET_SIZE(inode, (offset + size));
    ICACHE_SET_DIRTY(inode);
    DEBUG("write done");

    //    ASSERT(ret == size);
    return ret;
}