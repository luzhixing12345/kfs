
struct sub_command {
    char *name;                              // 绑定的名字
    int (*fp)(int argc, const char **argv);  // sub main fp
};

int add_main(int argc, const char **argv);
int status_main(int argc, const char **argv);
int log_main(int argc, const char **argv);
int defrag_main(int argc, const char **argv);
int restore_main(int argc, const char **argv);