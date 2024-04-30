

#include <sys/stat.h>
#include <unistd.h>

#define print_info(...) printf("[KFS INFO] " __VA_ARGS__)
#define print_error(...) printf("[KFS ERROR] " __VA_ARGS__)
#define print_debug(...) printf("[KFS DEBUG] " __VA_ARGS__)
#define print_warning(...) printf("[KFS WARNING] " __VA_ARGS__)

#define RWX (S_IRWXU | S_IRWXG | S_IRWXO)
#define RW (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define ROOT "/"

// file or dir
struct node {
    char *filename;  // 文件或目录的名称
    char *contents;  // 文件的内容.对于目录,这个字段可能不被使用或为NULL
    char *path;      // 节点的完整路径名,可能是相对路径或绝对路径
    mode_t mode;  // 文件或目录的类型和权限,如S_IFREG表示普通文件,S_IRWXU表示所有者有读写执行权限
    uid_t uid;               // 拥有该文件或目录的用户ID
    gid_t gid;               // 拥有该文件或目录的组ID
    struct node *parent;     // 指向其父节点的指针,如果该节点是根目录,则可能为NULL
    struct node **children;  // 指向子节点数组的指针,存储其下所有子文件或目录的node结构体指针
    int child_count;         // 子节点的数量,即children数组中的元素数量
    time_t atime;            // 最后一次访问时间
    time_t mtime;            // 最后一次修改内容的时间
    time_t ctime;            // 状态改变时间,如权限、所有权的变更等
};

struct file_info *get_file_info(const char *name, struct file_info *files);