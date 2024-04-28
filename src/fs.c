
#include "fs.h"
#include <string.h>

// 获取文件信息的辅助函数
struct file_info *get_file_info(const char *name, struct file_info *files) {
    for (struct file_info *f = files; f->name != NULL; f++) {
        if (strcmp(name, f->name) == 0) {
            return f;
        }
    }
    return NULL;
}