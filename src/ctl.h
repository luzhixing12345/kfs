

#pragma once

#include <fcntl.h>
#include <stdint.h>

#define CTL_OFFSET_CONSTANT 4096

enum kfs_cmd { CMD_STATUS = 1, CMD_ADD, CMD_LOG };

int ctl_check(const char *path);
int ctl_read(char *buf, enum kfs_cmd cmd);
int ctl_write(const char *filename, enum kfs_cmd cmd);