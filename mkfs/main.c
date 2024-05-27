
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bitmap.h"
#include "config.h"
#include "disk.h"
#include "ext4/ext4.h"
#include "ext4/ext4_basic.h"
#include "ext4/ext4_inode.h"
#include "ext4/ext4_super.h"
#include "logging.h"

struct ext4_super_block sb;
struct ext4_group_desc *gdt;

int inode_create(uint32_t inode_idx, mode_t mode, uint64_t pblock, struct ext4_inode *inode) {
    memset(inode, 0, sizeof(struct ext4_inode));
    inode->i_mode = mode;
    time_t now = time(NULL);
    inode->i_atime = inode->i_ctime = inode->i_mtime = now;
    uid_t uid = getuid();
    gid_t gid = getgid();
    EXT4_INODE_SET_UID(inode, uid);
    EXT4_INODE_SET_GID(inode, gid);

    // dir has 2 links and others have 1 link
    inode->i_links_count = mode & S_IFDIR ? 2 : 1;

    // new created inode has 0 block and 0 size
    EXT4_INODE_SET_BLOCKS(inode, 0);
    uint64_t inode_size = mode & S_IFDIR ? BLOCK_SIZE : 0;
    EXT4_INODE_SET_SIZE(inode, inode_size);

    // enable ext4 extents flag
    inode->i_flags |= EXT4_EXTENTS_FL;

    // ext4 extent header in block[0-3]
    struct ext4_extent_header *eh = (struct ext4_extent_header *)(inode->i_block);
    eh->eh_magic = EXT4_EXT_MAGIC;
    eh->eh_entries = 1;
    eh->eh_max = EXT4_EXT_LEAF_EH_MAX;
    eh->eh_depth = 0;
    eh->eh_generation = EXT4_EXT_EH_GENERATION;

    // new inode has one ext4 extent
    struct ext4_extent *ee = (struct ext4_extent *)(eh + 1);
    ee->ee_block = 0;
    ee->ee_len = 1;
    ee->ee_start_hi = (pblock >> 32) & MASK_16;
    ee->ee_start_lo = (pblock & MASK_32);
    INFO("hi[%u] lo[%u]", ee->ee_start_hi, ee->ee_start_lo);

    // set other 3 ext extents to 0
    ee++;
    memset(ee, 0, 3 * sizeof(struct ext4_extent));
    INFO("new inode %u created", inode_idx);
    return 0;
}

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

    if (img_size < MKFS_EXT4_DISK_MIN_SIZE) {
        fprintf(stderr, "Image size is too small, at least %d bit is required\n", MKFS_EXT4_DISK_MIN_SIZE);
        return EXIT_FAILURE;
    }

    if (img_size / MKFS_EXT4_BLOCK_SIZE > 0 && img_size % MKFS_EXT4_BLOCK_SIZE != 0) {
        DEBUG("Image size is not a multiple of block size");
        img_size = (img_size / MKFS_EXT4_BLOCK_SIZE) * MKFS_EXT4_BLOCK_SIZE;
        INFO("Adjust image size to %llu", img_size);
    }

    // https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Blocks
    // For the special case of block group 0, the first 1024 bytes are unused, to allow for the installation
    // of x86 boot sectors and other oddities. The superblock will start at offset 1024 bytes, whichever block
    // that happens to be (usually 0).
    void *tmp_mem_area = calloc(1, BOOT_SECTOR_SIZE);
    disk_write(0, BOOT_SECTOR_SIZE, tmp_mem_area);

    // just fill the struct members of super block what we used in kfs
    // see op_init.c
    uint64_t block_count = img_size / MKFS_EXT4_BLOCK_SIZE;
    uint64_t group_count = (block_count + MKFS_EXT4_GROUP_BLOCK_CNT - 1) / MKFS_EXT4_GROUP_BLOCK_CNT;
    uint64_t inode_count = block_count * MKFS_EXT4_BLOCK_SIZE / MKFS_EXT4_INODE_RATIO;
    // each group inode number should be less than MKFS_EXT4_GROUP_BLOCK_CNT
    ASSERT((inode_count / group_count) <= MKFS_EXT4_GROUP_BLOCK_CNT);
    INFO("block_count: %llu, group_count: %llu, inode_count: %llu", block_count, group_count, inode_count);

    memset(&sb, 0, sizeof(struct ext4_super_block));
    sb.s_inodes_count = inode_count;
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
    uint64_t d_bitmap_start = 2 + MKFS_EXT4_RESV_DESC_CNT;
    uint64_t i_bitmap_start = d_bitmap_start + group_count;
    uint64_t inode_table_start = i_bitmap_start + group_count;
    uint64_t inode_table_len = sb.s_inodes_per_group * MKFS_EXT4_INODE_SIZE / MKFS_EXT4_BLOCK_SIZE;

    // resize to 4K zero block
    tmp_mem_area = realloc(tmp_mem_area, MKFS_EXT4_BLOCK_SIZE);
    memset(tmp_mem_area, 0, MKFS_EXT4_BLOCK_SIZE);

    for (uint32_t i = 0; i < group_count; i++) {
        INFO("group [%d] block bitmap[%lu], inode bitmap[%lu], inode table[%lu-%lu]",
             i,
             d_bitmap_start + i,
             i_bitmap_start + i,
             inode_table_start + i * inode_table_len,
             inode_table_start + (i + 1) * inode_table_len - 1);
        gdt[i].bg_block_bitmap_lo = (d_bitmap_start + i) & MASK_32;
        gdt[i].bg_block_bitmap_hi = (d_bitmap_start + i) >> 32;
        gdt[i].bg_inode_bitmap_lo = (i_bitmap_start + i) & MASK_32;
        gdt[i].bg_inode_bitmap_hi = (i_bitmap_start + i) >> 32;
        gdt[i].bg_inode_table_lo = (inode_table_start + i * inode_table_len) & MASK_32;
        gdt[i].bg_inode_table_hi = (inode_table_start + i * inode_table_len) >> 32;

        uint64_t free_blocks_count = MKFS_EXT4_GROUP_BLOCK_CNT;
        uint64_t free_inode_count = sb.s_inodes_per_group;
        // calculate left free block count
        if (i == group_count - 1) {
            free_blocks_count = block_count - (group_count - 1) * MKFS_EXT4_GROUP_BLOCK_CNT;
            free_inode_count = inode_count - (group_count - 1) * sb.s_inodes_per_group;
        } else if (i == 0) {
            free_blocks_count--;  // root data block
            free_inode_count--;   // root inode
        }
        gdt[i].bg_free_blocks_count_lo = free_blocks_count & MASK_32;
        gdt[i].bg_free_blocks_count_hi = free_blocks_count >> 32;
        gdt[i].bg_free_inodes_count_lo = free_inode_count & MASK_32;
        gdt[i].bg_free_inodes_count_hi = 0;  // sb.s_inodes_per_group is uint32_t so no need to shift?
        gdt[i].bg_used_dirs_count_lo = 0;
        gdt[i].bg_used_dirs_count_hi = 0;
        // i don't know the difference between this two struct members
        gdt[i].bg_itable_unused_lo = gdt[i].bg_free_inodes_count_lo;
        gdt[i].bg_itable_unused_hi = gdt[i].bg_free_inodes_count_hi;

        // bitmap must init with zero, and don't need to init inode table
        disk_write_block(d_bitmap_start + i, tmp_mem_area);
        disk_write_block(i_bitmap_start + i, tmp_mem_area);
    }
    // only write back group descriptors in the first group
    INFO("filling group descriptors");
    gdt[0].bg_used_dirs_count_lo += 1;  // root '/'
    disk_write(bg_off, group_count * sizeof(struct ext4_group_desc), gdt);

    // init fs
    //
    // TODO: actually ext4 has some special inode numbers(1-8), see ext4.h
    //       inode 1 and inode 2 is necessary, we don't care about others for now
    SET_BIT(tmp_mem_area, 0, 1);  // inode 1 for bad block(don't exsist)
    SET_BIT(tmp_mem_area, 1, 1);  // inode 2 for root block
    INFO("write back inode bitmap to disk");
    disk_write(BLOCKS2BYTES(i_bitmap_start), 2, tmp_mem_area);

    // control blocks alloced in group0
    uint64_t alloced_block_cnt = inode_table_start + group_count * inode_table_len;
    uint64_t data_block_start = alloced_block_cnt;
    alloced_block_cnt++;

    memset(tmp_mem_area, 0xff, alloced_block_cnt / 8);
    if (alloced_block_cnt % 8 != 0) {
        ((uint8_t *)tmp_mem_area)[alloced_block_cnt / 8] |= (1 << (alloced_block_cnt % 8)) - 1;
    }

    DEBUG("alloc %lu data blocks", alloced_block_cnt);
    disk_write_block(d_bitmap_start, tmp_mem_area);
    INFO("finish bitmap change");

    // create inode
    struct ext4_inode *inode = (struct ext4_inode *)tmp_mem_area;
    mode_t mode = umask(0) ^ 0777;

    INFO("create root inode");
    inode_create(EXT4_ROOT_INO, S_IFDIR | mode, data_block_start, inode);

    INFO("write back root inode to disk");
    disk_write(BLOCKS2BYTES(inode_table_start) + (EXT4_ROOT_INO - 1) * MKFS_EXT4_INODE_SIZE,
               MKFS_EXT4_INODE_SIZE,
               tmp_mem_area);

    // create dentry
    INFO("create root dentry");
    // create dentry tail
    struct ext4_dir_entry_tail *de_tail = (struct ext4_dir_entry_tail *)(tmp_mem_area + BLOCK_SIZE - EXT4_DE_TAIL_SIZE);
    de_tail->det_reserved_zero1 = 0;
    de_tail->det_rec_len = EXT4_DE_TAIL_SIZE;
    de_tail->det_reserved_zero2 = 0;
    de_tail->det_reserved_ft = EXT4_FT_DIR_CSUM;
    de_tail->det_checksum = 0;  // TODO: checksum

    // .
    struct ext4_dir_entry_2 *de_dot = (struct ext4_dir_entry_2 *)(tmp_mem_area);
    de_dot->inode_idx = EXT4_ROOT_INO;
    de_dot->rec_len = EXT4_DE_DOT_SIZE;
    de_dot->name_len = 1;
    de_dot->file_type = EXT4_FT_DIR;
    strncpy(de_dot->name, ".", 1);
    de_dot->name[1] = 0;

    // ..
    struct ext4_dir_entry_2 *de_dot2 = (struct ext4_dir_entry_2 *)(tmp_mem_area + EXT4_DE_DOT_SIZE);
    de_dot2->inode_idx = EXT4_ROOT_INO;
    de_dot2->rec_len = BLOCK_SIZE - EXT4_DE_DOT_SIZE - EXT4_DE_TAIL_SIZE;
    de_dot2->name_len = 2;
    de_dot2->file_type = EXT4_FT_DIR;
    strncpy(de_dot2->name, "..", 2);
    de_dot2->name[2] = 0;

    INFO("create root dentry done");
    disk_write_block(data_block_start, tmp_mem_area);
    INFO("write back root dentry to disk");

    free(tmp_mem_area);
    free(gdt);
    return EXIT_SUCCESS;
}
