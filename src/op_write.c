#include <stddef.h>
#include <stdio.h>
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

    // if (offset != 0 && (ctl_check(path) || strcmp(path, "/.kfsctl") == 0)) {
    //     char result[1024];
    //     const char *last_slash = strrchr(path, '/');
    //     size_t length = last_slash - path;
    //     strncpy(result, path, length);
    //     result[length] = '\0';
    //     snprintf(result + length, sizeof(result) - length, "/%s", buf);
    //     DEBUG("ctl path: %s", result);
    //     ctl_write(result, offset / CTL_OFFSET_CONSTANT);
    //     return size;
    // }

    int ret = 0;
    size_t remain = size;

    struct ext4_inode *inode;
    uint32_t inode_idx;

    if (fi && fi->fh > 0) {
        if (inode_get_by_number(fi->fh, &inode) < 0) {
            DEBUG("fail to get inode %d", fi->fh);
            return -ENOENT;
        }
    } else {
        if (inode_get_by_path(path, &inode, &inode_idx) < 0) {
            DEBUG("fail to get inode %s", path);
            return -ENOENT;
        }
    }

    // check permissions
    if (inode_check_permission(inode, WRITE) < 0) {
        ERR("Permission denied");
        return -EACCES;
    }

    uint64_t start_lblock = offset / BLOCK_SIZE;
    uint64_t end_lblock = (offset + size - 1) / BLOCK_SIZE;
    uint32_t start_block_off = offset % BLOCK_SIZE;

    uint32_t extend_len = 0;

    if (start_lblock == end_lblock) {
        // only one block to write
        uint64_t pblock_idx;
        if (EXT4_INODE_GET_BLOCKS(inode) == 0) {
            DEBUG("inode %d has no blocks", inode_idx);
            pblock_idx = bitmap_pblock_find(inode_idx, EXT4_INODE_PBLOCK_NUM);
            inode_init_pblock(inode, pblock_idx);
        } else {
            pblock_idx = inode_get_data_pblock(inode, start_lblock, &extend_len);
        }
        ret = disk_write(BLOCKS2BYTES(pblock_idx) + start_block_off, size, (void *)buf);
        if (ret != size) {
            return -EIO;
        }
        remain -= ret;
    } else {
        // FIXME : 读出数据与原始数据不一致
        uint32_t extent_len = 0;
        for (uint64_t lblock = start_lblock; lblock <= end_lblock; ++lblock) {
            uint32_t block_off = (lblock == start_lblock) ? start_block_off : 0;
            size_t block_size =
                (lblock == end_lblock) ? (offset + size - (lblock * BLOCK_SIZE)) : (BLOCK_SIZE - block_off);

            assert(block_size <= BLOCK_SIZE);

            uint64_t pblock = extent_get_pblock(inode->i_block, lblock, &extent_len);

            ret = disk_write(BLOCKS2BYTES(pblock) + block_off, block_size, (void *)buf);

            if (ret != block_size) {
                return -EIO;
            }
            remain -= ret;
            buf += block_size;
        }
    }

    EXT4_INODE_SET_SIZE(inode, (offset + size));
    ICACHE_SET_DIRTY(inode);
    DEBUG("write done");

    ASSERT(remain == 0);
    return ret;
}