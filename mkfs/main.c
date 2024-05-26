
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_basic.h"
#include "ext4/ext4_super.h"
#include "logging.h"

struct ext4_super_block sb;
struct ext4_group_desc *gdt;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (disk_open(argv[1]) < 0) {
        fprintf(stderr, "disk_open: %s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    if (logging_open("/dev/stdout") < 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return EXIT_FAILURE;
    }

    uint64_t img_size = disk_size();
    DEBUG("Image size: %llu", img_size);

    // TODO: check if disk is big enough
    if (img_size < MKFS_EXT4_DISK_MIN_SIZE) {
        fprintf(stderr, "Image size is too small, at least %d is required\n", MKFS_EXT4_DISK_MIN_SIZE);
        return EXIT_FAILURE;
    }

    if (img_size / MKFS_EXT4_BLOCK_SIZE > 0 && img_size % MKFS_EXT4_BLOCK_SIZE != 0) {
        DEBUG("Image size is not a multiple of block size");
        img_size = (img_size / MKFS_EXT4_BLOCK_SIZE) * MKFS_EXT4_BLOCK_SIZE;
        INFO("Adjust image size to %llu", img_size);
    }

    // TODO: what should be inside boot sector? seems x86 specific
    void *zero_fill = calloc(1, BOOT_SECTOR_SIZE);
    disk_write(0, BOOT_SECTOR_SIZE, zero_fill);

    // just fill the struct members of super block what we used in kfs
    // see op_init.c
    uint64_t block_count = img_size / MKFS_EXT4_BLOCK_SIZE;
    uint64_t group_count = block_count / MKFS_EXT4_GROUP_BLOCK_CNT;
    uint64_t inode_count = block_count * MKFS_EXT4_BLOCK_SIZE / MKFS_EXT4_INODE_RATIO;
    INFO("block_count: %llu, group_count: %llu, inode_count: %llu", block_count, group_count, inode_count);

    memset(&sb, 0, sizeof(struct ext4_super_block));
    sb.s_magic = EXT4_S_MAGIC;
    sb.s_log_block_size = MKFS_EXT4_LOG_BLOCK_SIZE;
    sb.s_blocks_count_hi = block_count >> 32;
    sb.s_blocks_count_lo = block_count & MASK_32;
    sb.s_blocks_per_group = MKFS_EXT4_GROUP_BLOCK_CNT;
    sb.s_inode_size = MKFS_EXT4_INODE_SIZE;
    sb.s_inodes_per_group = inode_count / group_count;
    sb.s_desc_size = MKFS_EXT4_DESC_SIZE;

    disk_write(BOOT_SECTOR_SIZE, sizeof(struct ext4_super_block), &sb);
    INFO("finish filling super block");
    INFO("BLOCK SIZE: %i", EXT4_BLOCK_SIZE(sb));
    INFO("N BLOCK GROUPS: %i", EXT4_N_BLOCK_GROUPS(sb));
    INFO("INODE SIZE: %i", EXT4_S_INODE_SIZE(sb));
    INFO("INODES PER GROUP: %i", EXT4_INODES_PER_GROUP(sb));
    INFO("BLOCKS PER GROUP: %i", EXT4_BLOCKS_PER_GROUP(sb));

    // fill gdt & bitmap & inode table
    DEBUG("start filling gdt & bitmap & inode table");
    uint64_t bg_off = ALIGN_TO_BLOCKSIZE(BOOT_SECTOR_SIZE + sizeof(struct ext4_super_block));
    gdt = calloc(group_count, sizeof(struct ext4_group_desc));

    // TODO: how to calculate reserverd GDT length?
    // MKFS_EXT4_RESV_DESC_CNT
    // start from block 2 because block 0 is for superblock, block 1 for gdt
    uint64_t d_bitmap_pblk = 2 + MKFS_EXT4_RESV_DESC_CNT;
    uint64_t i_bitmap_pblk = d_bitmap_pblk + group_count;
    uint64_t inode_table_pblk = i_bitmap_pblk + group_count;
    uint64_t inode_table_len = sb.s_inodes_per_group * MKFS_EXT4_INODE_SIZE / MKFS_EXT4_BLOCK_SIZE;
    zero_fill = realloc(zero_fill, MKFS_EXT4_BLOCK_SIZE);

    for (uint32_t i = 0; i < group_count; i++) {
        INFO("group [%d] block bitmap[%lu], inode bitmap[%lu], inode table[%lu:%lu]",
             i,
             d_bitmap_pblk,
             i_bitmap_pblk,
             inode_table_pblk,
             inode_table_len);
        gdt[i].bg_block_bitmap_lo = d_bitmap_pblk & MASK_32;
        gdt[i].bg_block_bitmap_hi = d_bitmap_pblk >> 32;
        gdt[i].bg_inode_bitmap_lo = i_bitmap_pblk & MASK_32;
        gdt[i].bg_inode_bitmap_hi = i_bitmap_pblk >> 32;
        gdt[i].bg_inode_table_lo = inode_table_pblk & MASK_32;
        gdt[i].bg_inode_table_hi = inode_table_pblk >> 32;
        gdt[i].bg_free_blocks_count_lo = MKFS_EXT4_GROUP_BLOCK_CNT & MASK_32;
        gdt[i].bg_free_blocks_count_hi = MKFS_EXT4_GROUP_BLOCK_CNT >> 32;
        gdt[i].bg_free_inodes_count_lo = sb.s_inodes_per_group & MASK_32;
        gdt[i].bg_free_inodes_count_hi = 0;  // sb.s_inodes_per_group is uint32_t so no need to shift?
        gdt[i].bg_used_dirs_count_lo = 0;
        gdt[i].bg_used_dirs_count_hi = 0;
        // i don't know the difference between this two struct members
        gdt[i].bg_itable_unused_lo = gdt[i].bg_free_inodes_count_lo;
        gdt[i].bg_itable_unused_hi = gdt[i].bg_free_inodes_count_hi;

        // bitmap must init with zero, and don't need to init inode table
        disk_write_block(d_bitmap_pblk, zero_fill);
        disk_write_block(i_bitmap_pblk, zero_fill);

        d_bitmap_pblk++;
        i_bitmap_pblk++;
        inode_table_pblk += inode_table_len;
    }
    // only write back group descriptors in the first group
    disk_write(bg_off, group_count * sizeof(struct ext4_group_desc), gdt);
    INFO("finish filling group descriptors");

    // init fs

    return EXIT_SUCCESS;
}
