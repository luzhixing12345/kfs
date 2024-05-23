
#include <stdlib.h>

#include "bitmap.h"
#include "cache.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_basic.h"
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
            disk_write(i_bitmap.group[i].off, EXT4_INODES_PER_GROUP(sb) / 8, &i_bitmap.group[i].bitmap);
        }
        if (d_bitmap.group[i].status == BITMAP_S_DIRTY) {
            INFO("write back dirty d_bitmap %d", i);
            disk_write(d_bitmap.group[i].off, EXT4_BLOCKS_PER_GROUP(sb) / 8, &d_bitmap.group[i].bitmap);
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
        if (icache->entries[i].status == I_CACHED_S_DIRTY) {
            INFO("write back dirty inode %d", icache->entries[i].inode_idx);
            disk_write(
                inode_get_offset(icache->entries[i].inode_idx), sizeof(struct ext4_inode), &icache->entries[i].inode);
        }
    }
    free(icache->entries);
    free(icache);
    INFO("free icache done");

    free(gdt);
    INFO("free group descriptors done");

    disk_write(BOOT_SECTOR_SIZE, sizeof(struct ext4_super_block), &sb);
    INFO("write back super block done");
    INFO("ext4 fuse fs destory done");
}