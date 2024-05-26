
#define KB                        1024
#define MB                        (KB * 1024)

#define MKFS_EXT4_DISK_MIN_SIZE   (128 * MB)

#define MKFS_EXT4_LOG_BLOCK_SIZE  12
#define MKFS_EXT4_BLOCK_SIZE      ((uint64_t)1 << MKFS_EXT4_LOG_BLOCK_SIZE)  // 4096
#define MKFS_EXT4_GROUP_BLOCK_CNT (MKFS_EXT4_BLOCK_SIZE * 8)
#define MKFS_EXT4_INODE_SIZE      256
#define MKFS_EXT4_INODE_RATIO     16384
#define MKFS_EXT4_DESC_SIZE       32
#define MKFS_EXT4_RESV_DESC_CNT   0