
#include <stdint.h>

#define CTL_OFFSET_CONSTANT 4096

enum kfs_cmd { CMD_STATUS = 1, CMD_ADD, CMD_LOG };

int ctl_init();
int ctl_destroy();

int ctl_read(char *buf, enum kfs_cmd cmd);
int ctl_write(const char *filename, enum kfs_cmd cmd);
int ctl_cmd(enum kfs_cmd cmd, const char *filename);
int ctl_add(const char *filename);

