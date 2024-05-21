
#include <stdlib.h>
#include "ops.h"
#include "logging.h"
#include "ext4/ext4.h"
#include "disk.h"

void op_destory(void *data) {
    DEBUG("ext4 fuse fs destory");
    // TODO free dcache
    free(gdt);
    disk_write(BOOT_SECTOR_SIZE, sizeof(struct ext4_super_block), &sb);
}