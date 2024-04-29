

#include <sys/stat.h>
#include <unistd.h>

#define print_info(...) printf("[KFS INFO] " __VA_ARGS__)

// file or dir
struct node {
    char *filename;  // file or directory name
    char *contents;  // file contents
    char *path;
    // int type; // 0 for file,1 for directory
    mode_t mode;  // The type and permissions of the file or directory
    uid_t uid;
    gid_t gid;
    struct node *parent;     // parent node
    struct node **children;  // children nodes
    int child_count;         // numbers of children nodes
    time_t atime;            // access time
    time_t mtime;            // modification time
    time_t ctime;            // state change time
};


struct file_info *get_file_info(const char *name, struct file_info *files);