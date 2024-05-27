#define KiB                       1024
#define MiB                       (KiB * 1024)

#define MKFS_EXT4_DISK_MIN_SIZE   (512 * KiB)

#define MKFS_EXT4_LOG_BLOCK_SIZE  2
#define MKFS_EXT4_BLOCK_SIZE      ((uint64_t)1 << (MKFS_EXT4_LOG_BLOCK_SIZE + 10))  // 4096
#define MKFS_EXT4_GROUP_BLOCK_CNT (MKFS_EXT4_BLOCK_SIZE * 8)
#define MKFS_EXT4_INODE_SIZE      256
#define MKFS_EXT4_INODE_RATIO     (MKFS_EXT4_BLOCK_SIZE * 4)
#define MKFS_EXT4_DESC_SIZE       64
#define MKFS_EXT4_RESV_DESC_CNT   0
