/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_extents.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

static int get_link_dest(struct ext4_inode *inode, char *buf, size_t bufsize) {
    uint64_t inode_size = EXT4_INODE_SIZE(inode);

    // a small perf for symbolic links
    // if the link destination fits in the inode, we copy it directly
    if (inode_size <= sizeof(inode->i_block)) {
        /* <= 60 bytes: Link destination fits in inode */
        DEBUG("read link destination fits in inode");
        memcpy(buf, &inode->i_block, inode_size);
    } else {
        // TODO: check
        if (inode_size >= bufsize) {
            // bufsize because we need to add a null terminator
            DEBUG("inode size %lu >= bufsize %lu, need to truncate", inode_size, bufsize);
        }
        uint64_t pblock = inode_get_data_pblock(inode, 0, NULL);
        char *block_data = malloc(EXT4_BLOCK_SIZE(sb));
        disk_read_block(pblock, (uint8_t *)block_data);
        strncpy(buf, block_data, bufsize - 1);
        free(block_data);
    }

    buf[inode_size] = 0;
    return inode_size + 1;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 */
int op_readlink(const char *path, char *buf, size_t bufsize) {
    struct ext4_inode *inode;
    DEBUG("readlink %s with bufsize %d", path, bufsize);

    if (inode_get_by_path(path, &inode, NULL) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }
    if (!S_ISLNK(inode->i_mode)) {
        DEBUG("inode %s is not a symlink", path);
        return -EINVAL;
    }

    get_link_dest(inode, buf, bufsize);
    DEBUG("Link resolved: %s => %s", path, buf);
    return 0;
}
