
#ifndef EXT4_EXTENTS_H
#define EXT4_EXTENTS_H

#include "ext4_basic.h"

// In ext4, the file to logical block map has been replaced with an extent tree. Under the old scheme, allocating a
// contiguous run of 1,000 blocks requires an indirect block to map all 1,000 entries; with extents, the mapping is
// reduced to a single struct ext4_extent with ee_len = 1000. If flex_bg is enabled, it is possible to allocate very
// large files with a single extent, at a considerable reduction in metadata block use, and some improvement in disk
// efficiency. The inode must have the extents flag (0x80000) flag set for this feature to be in use.

// Extents are arranged as a tree. Each node of the tree begins with a struct ext4_extent_header. If the node is an
// interior node (eh.eh_depth > 0), the header is followed by eh.eh_entries instances of struct ext4_extent_idx; each of
// these index entries points to a block containing more nodes in the extent tree. If the node is a leaf node
// (eh.eh_depth == 0), then the header is followed by eh.eh_entries instances of struct ext4_extent; these instances
// point to the file's data blocks. The root node of the extent tree is stored in inode.i_block, which allows for the
// first four extents to be recorded without the use of extra metadata blocks.

/*
 * ext4_inode has i_block array (60 bytes total).
 * The first 12 bytes store ext4_extent_header;
 * the remainder stores an array of ext4_extent.
 */

/*
 * This is the extent on-disk structure.
 * It's used at the bottom of the tree.
 */
struct ext4_extent {
    __le32 ee_block;    /* First file block number that this extent covers. */
    __le16 ee_len;      /* Number of blocks covered by extent. If the value of this field is <= 32768, the extent is
                           initialized. If the value of the field is > 32768, the extent is uninitialized and the actual
                           extent length is ee_len - 32768. Therefore, the maximum length of a initialized extent is 32768
                           blocks, and the maximum length of an uninitialized extent is 32767. */
    __le16 ee_start_hi; /* Upper 16-bits of the block number to which this extent points. */
    __le32 ee_start_lo; /* Lower 32-bits of the block number to which this extent points. */
};

#define EXT4_EXT_PADDR(ext) ((((uint64_t)(ext).ee_start_hi) << 32) | (ext).ee_start_lo)
#define EXT4_EXT_SET_PADDR(ext, addr) \
    ((ext).ee_start_hi = ((addr) >> 32) & MASK_16, (ext).ee_start_lo = (addr & MASK_32))

/*
 * This is index on-disk structure.
 * It's used at all the levels except the bottom.
 */
struct ext4_extent_idx {
    __le32 ei_block;   /* index covers logical blocks from 'block' */
    __le32 ei_leaf_lo; /* pointer to the physical block of the next *
                        * level. leaf or next index could be there */
    __le16 ei_leaf_hi; /* high 16 bits of physical block */
    __u16 ei_unused;
};

#define EXT4_EXT_LEAF_ADDR(idx) ((((uint64_t)(idx)->ei_leaf_hi) << 32) | (idx)->ei_leaf_lo)
#define EXT4_EXT_LEAF_SET_ADDR(idx, addr) \
    ((idx)->ei_leaf_hi = ((addr) >> 32) & MASK_16, (idx)->ei_leaf_lo = (addr & MASK_32))

/*
 * Each block (leaves and indexes), even inode-stored has header.
 */
// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Extent_Tree
struct ext4_extent_header {
    __le16 eh_magic;      /* 魔数,用于识别扩展索引的格式 */
    __le16 eh_entries;    /* 有效的条目数量,表示当前有多少条目在使用 */
    __le16 eh_max;        /* 存储空间的最大容量,以条目数计 */
    __le16 eh_depth;      /* 树的深度,表示是否有实际的底层块 */
    __le32 eh_generation; /* 扩展索引树的版本号,用于确保一致性 */
};

#define EXT4_EXT_MAGIC         0xF30A  // extent header magic number
#define EXT4_EXT_LEAF_EH_MAX   4
#define EXT4_EXT_EH_GENERATION 0
#define EXT4_MAX_EXTENT_DEPTH  5
#define EXT4_EXT_EH_MAX                                                                   \
    ((BLOCK_SIZE - sizeof(struct ext4_extent_header) - sizeof(struct ext4_extent_tail)) / \
     sizeof(struct ext4_extent))  // 340 if 4096

struct ext4_extent_tail {
    __le32 eb_checksum;  // Checksum of the extent block, crc32c(uuid+inum+igeneration+extentblock)
};

#endif
