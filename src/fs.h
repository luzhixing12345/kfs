

#include <sys/stat.h>
#include <unistd.h>

// 模拟的文件系统结构
struct kfs {
    char *files;  // 模拟文件系统中的文件,以字符串形式存储
};

// 定义文件系统中的文件和目录
struct file_info {
    const char *name;
    const mode_t mode;
    const char *data;
    size_t size;
};

struct file_info *get_file_info(const char *name, struct file_info *files);