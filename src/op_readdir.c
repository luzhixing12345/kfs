/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <string.h>

#include "cache.h"
#include "common.h"
#include "dentry.h"
#include "logging.h"
#include "ops.h"

extern struct dcache *dcache;

static char *get_printable_name(char *s, struct ext4_dir_entry_2 *entry) {
    memcpy(s, entry->name, entry->name_len);
    s[entry->name_len] = 0;
    return s;
}

int op_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
               enum fuse_readdir_flags flags) {
    DEBUG("readdir path %s offset %d", path, offset);

    UNUSED(fi);
    char name_buf[EXT4_NAME_LEN + 1];
    struct ext4_dir_entry_2 *de = NULL;
    struct ext4_inode *inode;

    /* We can use inode_get_by_number, but first we need to implement opendir */
    uint32_t inode_idx;
    if (inode_get_by_path(path, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }

    dcache_init(inode, inode_idx);
    while ((de = dentry_next(inode, inode_idx, offset))) {
        offset += de->rec_len;

        if (de->inode_idx == 0 && de->name_len == 0) {
            // reach the ext4_dir_entry_tail
            ASSERT(((struct ext4_dir_entry_tail *)de)->det_reserved_ft == EXT4_FT_DIR_CSUM);
            INFO("inode %u last dentry", inode_idx);
            break;
        }
        DEBUG("pass dentry %s[%u:%u]", de->name, de->inode_idx, de->rec_len);
        /* Providing offset to the filler function seems slower... */
        get_printable_name(name_buf, de);
        if (name_buf[0]) {
            if (filler(buf, name_buf, NULL, offset, 0) != 0)
                break;
        }
    }

    return 0;
}
