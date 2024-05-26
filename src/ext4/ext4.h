
#include "ext4_basic.h"
#include "ext4_dentry.h"
#include "ext4_extents.h"
#include "ext4_inode.h"
#include "ext4_super.h"
#include "../common.h"

extern struct ext4_super_block sb;
extern struct ext4_group_desc *gdt;
extern struct bitmap i_bitmap;
extern struct bitmap d_bitmap;

#define BOOT_SECTOR_SIZE        0x400   // boot sector
#define EXT4_S_MAGIC            0xEF53  // ext4 magic number

#define BLOCK_SIZE              (EXT4_BLOCK_SIZE(sb))
#define BLOCKS2BYTES(__blks)    (((uint64_t)(__blks)) * BLOCK_SIZE)
#define BYTES2BLOCKS(__bytes)   ((__bytes) / BLOCK_SIZE + ((__bytes) % BLOCK_SIZE ? 1 : 0))

#define MALLOC_BLOCKS(__blks)   (malloc(BLOCKS2BYTES(__blks)))
#define ALIGN_TO_BLOCKSIZE(__n) (ALIGN_TO(__n, BLOCK_SIZE))
#define BIT0(bitmap, __n)       (!((bitmap)[(__n) / 8] & (1 << ((__n) % 8))))
#define SET_BIT(bitmap_ptr, __n, __v)                        \
    do {                                                     \
        char *byte_ptr = (char *)(bitmap_ptr) + ((__n) / 8); \
        if ((__v)) {                                         \
            *byte_ptr |= (1 << ((__n) % 8));                 \
        } else {                                             \
            *byte_ptr &= ~(1 << ((__n) % 8));                \
        }                                                    \
    } while (0)
/*
 * Macro-instructions used to manage group descriptors
 */
#define EXT4_DESC_SIZE_32BIT         32
#define EXT4_DESC_SIZE_64BIT         64
#define EXT4_DESC_SIZE(sb)           (sb.s_desc_size)
#define EXT4_S_INODE_SIZE(sb)        (sb.s_inode_size)
#define EXT4_BLOCK_SIZE(sb)          (((uint64_t)1) << (sb.s_log_block_size + 10))
#define EXT4_BLOCKS_PER_GROUP(sb)    (sb.s_blocks_per_group)
#define EXT4_CLUSTERS_PER_GROUP(sb)  (sb.s_clusters_per_group)
#define EXT4_DESC_PER_BLOCK(sb)      (sb.s_desc_per_block)
#define EXT4_INODES_PER_GROUP(sb)    (sb.s_inodes_per_group)
#define EXT4_DESC_PER_BLOCK_BITS(sb) (sb.s_desc_per_block_bits)

#define EXT4_DESC_INO_BITMAP(gdt)    (((uint64_t)(gdt.bg_inode_bitmap_hi) << 32) | gdt.bg_inode_bitmap_lo)
#define EXT4_DESC_BLOCK_BITMAP(gdt)  (((uint64_t)(gdt.bg_block_bitmap_hi) << 32) | gdt.bg_block_bitmap_lo)
#define EXT4_DESC_INODE_TABLE(gdt)   (((uint64_t)(gdt.bg_inode_table_hi) << 32) | gdt.bg_inode_table_lo)

#define EXT4_SUPER_GROUP_SIZE(sb)    (BLOCKS2BYTES(sb.s_blocks_per_group))
#define EXT4_BLOCK_COUNT(sb)         (((uint64_t)sb.s_blocks_count_hi << 32) | sb.s_blocks_count_lo)
#define EXT4_N_BLOCK_GROUPS(sb)      ((EXT4_BLOCK_COUNT(sb) + EXT4_BLOCKS_PER_GROUP(sb) - 1) / EXT4_BLOCKS_PER_GROUP(sb))

// dentry
#define EXT4_NEXT_DE(de)             ((struct ext4_dentry *)(de) + 1)