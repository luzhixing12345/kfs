

#pragma once

#include <stdint.h>

int ctl_init();
int ctl_check(uint32_t inode_idx);
int ctl_destroy();
int ctl_status(char *buf);