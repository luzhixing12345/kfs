/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include "extents.h"

#include <stdlib.h>

#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_extents.h"
#include "logging.h"

extern struct ext4_super_block sb;

/* Calculates the physical block from a given logical block and extent */
static uint64_t extent_get_block_from_ees(struct ext4_extent *ee, uint32_t n_ee, uint32_t lblock,
                                          uint32_t *extent_len) {
    uint32_t block_ext_index = 0;
    uint32_t block_ext_offset = 0;
    uint32_t i;

    DEBUG("Extent contains %d entries", n_ee);
    DEBUG("Looking for Logic Block %d", lblock);

    /* Skip to the right extent entry */
    for (i = 0; i < n_ee; i++) {
        // Check if the block is in this extent
        if (ee[i].ee_block + ee[i].ee_len > lblock) {
            DEBUG("Block found in extent [%d]", i);
            DEBUG("ee_block = %d, ee_len = %d, lblock = %d", ee[i].ee_block, ee[i].ee_len, lblock);
            block_ext_index = i;
            block_ext_offset = lblock - ee[i].ee_block;
            if (extent_len) {
                *extent_len = ee[i].ee_len;
            }
            break;
        }
        DEBUG("Block not found in extent %d [%d-%d]", i, ee[i].ee_block, ee[i].ee_block + ee[i].ee_len);
    }

    if (n_ee == i) {
        DEBUG("Extent [%d] doesn't contain block", block_ext_index);
        return 0;
    } else {
        DEBUG("Block located [%d:%d]", block_ext_index, block_ext_offset);
        return EXT4_EXT_PADDR(ee[block_ext_index]) + block_ext_offset;
    }
}

/* Fetches a block that stores extent info and returns an array of extents
 * _with_ its header. */
static void *extent_get_extents_in_block(uint32_t pblock) {
    // first read the extent header
    void *exts = malloc(sizeof(struct ext4_extent_header));
    disk_read(BLOCKS2BYTES(pblock), sizeof(struct ext4_extent_header), exts);
    ASSERT(((struct ext4_extent_header *)exts)->eh_magic == EXT4_EXT_MAGIC);
    ASSERT(((struct ext4_extent_header *)exts)->eh_max <= EXT4_EXT_EH_MAX);

    // resize exts to hold all the extents
    uint32_t extents_len = ((struct ext4_extent_header *)exts)->eh_entries * sizeof(struct ext4_extent);
    exts = realloc(exts, sizeof(struct ext4_extent_header) + extents_len);

    // read the extents
    disk_read(BLOCKS2BYTES(pblock) + sizeof(struct ext4_extent_header),
              extents_len,
              exts + sizeof(struct ext4_extent_header));

    return exts;
}

/* Returns the physical block number */
uint64_t extent_get_pblock(void *extents, uint32_t lblock, uint32_t *extent_len) {
    struct ext4_extent_header *eh = extents;
    struct ext4_extent *ee_array;
    uint64_t ret;

    ASSERT(eh->eh_magic == EXT4_EXT_MAGIC);
    ASSERT(eh->eh_max <= EXT4_EXT_LEAF_EH_MAX);
    DEBUG("reading inode extent, depth = %d", eh->eh_depth);

    if (eh->eh_depth == 0) {
        // Leaf inode, direct block
        // 1 extent header + 4 extents
        ee_array = extents + sizeof(struct ext4_extent_header);
        ret = extent_get_block_from_ees(ee_array, eh->eh_entries, lblock, extent_len);
    } else {
        struct ext4_extent_idx *ei_array = extents + sizeof(struct ext4_extent_header);
        struct ext4_extent_idx *recurse_ei = NULL;

        for (int i = 0; i < eh->eh_entries; i++) {
            if (ei_array[i].ei_block > lblock) {
                break;
            }

            recurse_ei = &ei_array[i];
        }

        ASSERT(recurse_ei != NULL);

        void *leaf_extents = extent_get_extents_in_block(EXT4_EXT_LEAF_ADDR(recurse_ei));
        ret = extent_get_pblock(leaf_extents, lblock, extent_len);
        free(leaf_extents);
    }

    return ret;
}
