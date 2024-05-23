
#include <stdlib.h>

#include "cache.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_basic.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"

extern struct dcache *dcache;
extern struct icache *icache;

void op_destory(void *data) {
    DEBUG("ext4 fuse fs destory");
    // TODO free dcache
    free(gdt);
    uint32_t group_num = EXT4_N_BLOCK_GROUPS(sb);

    for (uint32_t i = 0; i < group_num; i++) {
        free(i_bitmap[i]);
    }
    free(i_bitmap);

    INFO("free inode bitmap done");

    for (uint32_t i = 0; i < group_num; i++) {
        free(d_bitmap[i]);
    }
    free(d_bitmap);

    INFO("free data bitmap done");
    free(dcache);
    INFO("free dcache done");

    // write back all the dirty inode
    for (int i = 0; i < icache->count; i++) {
        if (icache->entries[i].status == I_CACHED_S_DIRTY) {
            disk_write(inode_get_offset(icache->entries[i].inode_idx), sizeof(struct ext4_inode), &icache->entries[i].inode);
        }
    }
    free(icache->entries);
    free(icache);
    INFO("free icache done");
    disk_write(BOOT_SECTOR_SIZE, sizeof(struct ext4_super_block), &sb);
    INFO("write back super block done");
    INFO("ext4 fuse fs destory done");
}