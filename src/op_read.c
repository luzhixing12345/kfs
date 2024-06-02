/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>

#include "common.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

/* We truncate the read size if it exceeds the limits of the file. */
static size_t truncate_size(struct ext4_inode *inode, size_t size, size_t offset) {
    uint64_t inode_size = EXT4_INODE_SIZE(inode);

    DEBUG(
        "inode size = %lu"
        ", read [%lu:%lu]",
        inode_size,
        offset,
        offset + size);

    if (inode_size == 0) {
        INFO("empty file, returning 0");
        return 0;
    }

    if (offset >= inode_size) {
        ERR("Offset %lu is greater than inode size %lu", offset, inode_size);
        return -EINVAL;
    }

    if ((offset + size) >= inode_size) {
        DEBUG("Truncating read(2) to %" PRIu64 " bytes", inode_size);
        return inode_size - offset;
    }

    return size;
}

/**
 * @brief read the first block, make sure align to BLOCK SIZE
 *
 * @param inode
 * @param buf
 * @param size
 * @param offset
 * @return size_t read bytes
 */
static size_t first_read(struct ext4_inode *inode, char *buf, size_t size, off_t offset) {
    uint32_t start_lblock = offset / BLOCK_SIZE;
    /* Reason for the -1 is that offset = 0 and size = BLOCK_SIZE is all on the
     * same block.  Meaning that byte at offset + size is not actually read. */
    uint32_t end_lblock = (offset + (size - 1)) / BLOCK_SIZE;
    uint32_t start_block_off = offset % BLOCK_SIZE;

    uint64_t start_pblock = inode_get_data_pblock(inode, start_lblock, NULL);

    if (start_lblock == end_lblock) {
        // only one block to read, finished
        disk_read(BLOCKS2BYTES(start_pblock) + start_block_off, size, buf);
        return size;
    } else {
        // more than one block to read, read the first part of the first block if offset is not aligned
        size_t size_to_block_end = ALIGN_TO_BLOCKSIZE(offset) - offset;
        ASSERT((offset + size_to_block_end) % BLOCK_SIZE == 0);

        disk_read(BLOCKS2BYTES(start_pblock) + start_block_off, size_to_block_end, buf);
        return size_to_block_end;
    }
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 */
int op_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // TODO direct_io
    size_t un_offset = (size_t)offset;
    struct ext4_inode *inode;
    size_t ret = 0;

    ASSERT(offset >= 0);
    ASSERT(fi->fh > 0);

    DEBUG("read [%lu:%lu] from %s", offset, offset + size, path);
    if (inode_get_by_number(fi->fh, &inode) < 0) {
        DEBUG("fail to get inode %d", fi->fh);
        return -ENOENT;
    }

    // Truncate the read size if it exceeds the limits of the file.
    size = truncate_size(inode, size, un_offset);
    if (size < 0) {
        ERR("Failed to truncate read(2) size");
        return size;
    } else if (size == 0) {
        DEBUG("file is empty, return 0");
        return 0;
    }
    ret = first_read(inode, buf, size, un_offset);
    INFO("first read %d bytes", ret);
    if (ret == size) {
        // all bytes read
        return ret;
    }

    buf += ret;
    un_offset += ret;
    ASSERT(un_offset % BLOCK_SIZE == 0);  // after first read, un_offset should be block aligned
    INFO("left to read %d bytes from offset %d", size - ret, un_offset);

    uint32_t extent_len;
    uint64_t pblock;
    size_t read_bytes;
    for (unsigned int lblock = un_offset / BLOCK_SIZE; size > ret; lblock += extent_len) {
        pblock = inode_get_data_pblock(inode, lblock, &extent_len);
        read_bytes = extent_len * BLOCK_SIZE;

        // last read
        if (size - ret <= read_bytes) {
            read_bytes = size - ret;
            INFO("last read %d bytes", read_bytes);
        }

        ret += disk_read(BLOCKS2BYTES(pblock), read_bytes, buf);
        buf += read_bytes;
        DEBUG("Read %zd/%zd bytes from %d consecutive blocks", ret, size, extent_len);
    }

    /* We always read as many bytes as requested (after initial truncation) */
    ASSERT(size == ret);
    return ret;
}
