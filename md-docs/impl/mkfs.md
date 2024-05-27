
# mkfs

本项目的 mkfs 相当的精简, 只完成了一个比较简单的 ext4 磁盘格式化

您可以使用如下命令编译并且格式化一个磁盘

```bash
make mkfs
./mkfs/mkfs disk.img
```

> 将会输出大量的信息

```bash
[debug][main:82] Image size: 1048576000
[debug][__disk_write:129] Disk Write: 0x0 +0x400 [main:100]
[info ][main:109] block_count: 256000, group_count: 8, inode_count: 64000
[debug][__disk_write:129] Disk Write: 0x400 +0x400 [main:121]
[info ][main:122] finish filling super block
[info ][main:123] BLOCK SIZE: 4096
[info ][main:124] N BLOCK GROUPS: 8
[info ][main:125] INODE SIZE: 256
[info ][main:126] INODES PER GROUP: 8000
[info ][main:127] BLOCKS PER GROUP: 32768
[debug][main:130] start filling gdt & bitmap & inode table
[info ][main:147] group [0] block bitmap[2], inode bitmap[10], inode table[18-517]
[debug][__disk_write:129] Disk Write: 0x2000 +0x1000 [main:178]
[debug][__disk_write:129] Disk Write: 0xa000 +0x1000 [main:179]
[info ][main:147] group [1] block bitmap[3], inode bitmap[11], inode table[518-1017]
[debug][__disk_write:129] Disk Write: 0x3000 +0x1000 [main:178]
[debug][__disk_write:129] Disk Write: 0xb000 +0x1000 [main:179]
[info ][main:147] group [2] block bitmap[4], inode bitmap[12], inode table[1018-1517]
[debug][__disk_write:129] Disk Write: 0x4000 +0x1000 [main:178]
[debug][__disk_write:129] Disk Write: 0xc000 +0x1000 [main:179]
[info ][main:147] group [3] block bitmap[5], inode bitmap[13], inode table[1518-2017]
[debug][__disk_write:129] Disk Write: 0x5000 +0x1000 [main:178]
[debug][__disk_write:129] Disk Write: 0xd000 +0x1000 [main:179]
[info ][main:147] group [4] block bitmap[6], inode bitmap[14], inode table[2018-2517]
[debug][__disk_write:129] Disk Write: 0x6000 +0x1000 [main:178]
[debug][__disk_write:129] Disk Write: 0xe000 +0x1000 [main:179]
[info ][main:147] group [5] block bitmap[7], inode bitmap[15], inode table[2518-3017]
[debug][__disk_write:129] Disk Write: 0x7000 +0x1000 [main:178]
[debug][__disk_write:129] Disk Write: 0xf000 +0x1000 [main:179]
[info ][main:147] group [6] block bitmap[8], inode bitmap[16], inode table[3018-3517]
[debug][__disk_write:129] Disk Write: 0x8000 +0x1000 [main:178]
[debug][__disk_write:129] Disk Write: 0x10000 +0x1000 [main:179]
[info ][main:147] group [7] block bitmap[9], inode bitmap[17], inode table[3518-4017]
[debug][__disk_write:129] Disk Write: 0x9000 +0x1000 [main:178]
[debug][__disk_write:129] Disk Write: 0x11000 +0x1000 [main:179]
[info ][main:182] filling group descriptors
[debug][__disk_write:129] Disk Write: 0x1000 +0x200 [main:183]
[info ][main:191] write back inode bitmap to disk
[debug][__disk_write:129] Disk Write: 0xa000 +0x2 [main:192]
[debug][main:204] alloc 4019 data blocks
[debug][__disk_write:129] Disk Write: 0x2000 +0x1000 [main:205]
[info ][main:206] finish bitmap change
[info ][main:212] create root inode
[info ][inode_create:56] hi[0] lo[4018]
[info ][inode_create:61] new inode 2 created
[info ][main:215] write back root inode to disk
[debug][__disk_write:129] Disk Write: 0x12100 +0x100 [main:216]
[info ][main:221] create root dentry
[info ][main:248] create root dentry done
[debug][__disk_write:129] Disk Write: 0xfb2000 +0x1000 [main:249]
[info ][main:250] write back root dentry to disk
```

## 磁盘分区

ext4 的磁盘布局就是按照 [ext4](../fs/ext4.md) 中描述的结构, 每一个块组的构成如下

![20240516100550](https://raw.githubusercontent.com/learner-lu/picbed/master/20240516100550.png)

整个磁盘由很多个块组构成, ext4 采用 flex group 的存储方案, 如下图所示

![20240516114033](https://raw.githubusercontent.com/learner-lu/picbed/master/20240516114033.png)

格式化流程主要分为如下几步

1. 读取整个磁盘大小, 根据 `MKFS_EXT4_BLOCK_SIZE` `MKFS_EXT4_INODE_SIZE` 来初步划分, 根据 `MKFS_EXT4_INODE_RATIO` 来确定 inode:blocks 的比例(默认 16384, 同 /etc/mke2fs.conf). 确定 block_count, group_count, inode_count
2. 初始化 boot sector 和 superblock, 完成对应字段的填写, 在对应位置写入磁盘
3. 根据 group_num 找到 d_bitmap/i_bitmap/inode-table 的起始位置, 确定 inode_table_len, 初始化并写入磁盘
4. 将上述 bitmap 信息填入 gdt, 初始化并写入磁盘
5. 初始化 root inode, 找到对应 inode_table 并写入, 初始化并写入磁盘
6. 找到一个空 datablock 并初始化 root dentry, 创建 ./.. 和 tail, 写入磁盘

gdt 初始化时最后一个组的 block 和 inode 数量可能不满, 需要单独计算

```c
for (i = 0; i < group_count; i++) {
    INFO("group [%d] block bitmap[%lu], inode bitmap[%lu], inode table[%lu-%lu]",
         i,
         d_bitmap_start + i,
         i_bitmap_start + i,
         inode_table_start + i * inode_table_len,
         inode_table_start + (i + 1) * inode_table_len - 1);
    // ...
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
    // ...
}
```

由于 flex 布局导致相关的控制信息全部保存在 group0 中, 因此需要在其 d_bitmap 中将占用过的位设置为 1

```c
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
```