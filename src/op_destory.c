
#include <stdlib.h>

#include "bitmap.h"
#include "cache.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_basic.h"
#include "ext4/ext4_super.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

extern struct dcache *dcache;
extern struct icache *icache;
extern struct bitmap i_bitmap;
extern struct bitmap d_bitmap;

void op_destory(void *data) {
    DEBUG("ext4 fuse fs destory");

    // write back all the dirty bitmaps
    for (int i = 0; i < i_bitmap.group_num; i++) {
        if (i_bitmap.group[i].status == BITMAP_S_DIRTY) {
            INFO("write back dirty i_bitmap %d", i);
            disk_write(i_bitmap.group[i].off, EXT4_INODES_PER_GROUP(sb) / 8, i_bitmap.group[i].bitmap);
        }
        if (d_bitmap.group[i].status == BITMAP_S_DIRTY) {
            INFO("write back dirty d_bitmap %d", i);
            disk_write(d_bitmap.group[i].off, EXT4_BLOCKS_PER_GROUP(sb) / 8, d_bitmap.group[i].bitmap);
        }
        free(i_bitmap.group[i].bitmap);
        free(d_bitmap.group[i].bitmap);
    }
    free(i_bitmap.group);
    free(d_bitmap.group);
    INFO("free inode & data bitmap done");

    free(dcache);
    INFO("free dcache done");

    // write back all the dirty inodes
    for (int i = 0; i < icache->count; i++) {
        if (ICACHE_IS_DIRTY(&icache->entries[i].inode)) {
            INFO("write back dirty inode %d", icache->entries[i].inode_idx);
            icache_write_back(&icache->entries[i]);
        }
    }
    free(icache->entries);
    free(icache);
    INFO("free icache done");

    // write back all the dirty group descriptors
    uint64_t bg_off = ALIGN_TO_BLOCKSIZE(BOOT_SECTOR_SIZE + sizeof(struct ext4_super_block));

    for (int i = 0; i < EXT4_N_BLOCK_GROUPS(sb); i++) {
        if (EXT4_GDT_DIRTY_FLAG(&gdt[i]) == EXT4_GDT_DIRTY) {
            INFO("write back dirty group descriptor %d", i);
            EXT4_GDT_SET_CLEAN(&gdt[i]);  // set it as clean before write back to disk
            disk_write(bg_off + i * EXT4_DESC_SIZE(sb), EXT4_DESC_SIZE(sb), &gdt[i]);
        }
    }
    free(gdt);
    INFO("free group descriptors done");

    disk_write(BOOT_SECTOR_SIZE, sizeof(struct ext4_super_block), &sb);
    INFO("write back super block done");
    INFO("ext4 fuse fs destory done");
}