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
    char name_buf[EXT4_NAME_LEN];
    struct ext4_dir_entry_2 *de = NULL;
    struct ext4_inode *inode;

    /* We can use inode_get_by_number, but first we need to implement opendir */
    uint32_t inode_idx;
    if (inode_get_by_path(path, &inode, &inode_idx) < 0) {
        DEBUG("fail to get inode %s", path);
        return -ENOENT;
    }

    dcache_init(inode, inode_idx);
    while ((de = dentry_next(inode, offset))) {
        offset += de->rec_len;

        if (!de->inode_idx) {
            /* It seems that is possible to have a dummy entry like this at the
             * begining of a block of dentries.  Looks like skipping is the
             * reasonable thing to do. */
            continue;
        }

        /* Providing offset to the filler function seems slower... */
        get_printable_name(name_buf, de);
        if (name_buf[0]) {
            if (filler(buf, name_buf, NULL, offset, 0) != 0)
                break;
        }
    }

    return 0;
}
