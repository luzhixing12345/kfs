
/*
 *Copyright (c) 2024 All rights reserved
 *@description: Terminal control program for kfs
 *@author: Zhixing Lu
 *@date: 2024-07-30
 *@email: luzhixing12345@163.com
 *@Github: luzhixing12345
 */

#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"
#include "xbox/xargparse.h"

const char *VERSION = "v0.0.1";

char *cmd;

int main(int argc, const char **argv) {
    argparse_option options[] = {XBOX_ARG_BOOLEAN(NULL, "-h", "--help", "show help information", NULL, "help"),
                                 XBOX_ARG_BOOLEAN(NULL, "-v", "--version", "show version", NULL, "version"),
                                 XBOX_ARG_STR_GROUP(&cmd, NULL, NULL, "sub command", NULL, "cmd"),
                                 XBOX_ARG_END()};

    XBOX_argparse parser;
    XBOX_argparse_init(&parser, options, XBOX_ARGPARSE_IGNORE_UNKNOWN);
    XBOX_argparse_describe(&parser,
                           "kfsctl",
                           "\nTerminal control program for kfs.\n\nSub commands:\n\tstatus: \tcheck fs status"
                           "\n\tadd \t\tmake snapshot for a file\n\tlog \t\tshow a file's snapshot log\n\trestore \trestore a file to a snapshot",
                           "Documentation: https://github.com/luzhixing12345/kfs/kfsctl/README.md\n");
    XBOX_argparse_parse(&parser, argc, argv);

    struct sub_command git_commands[] = {
        {"add", add_main},
        {"status", status_main},
        {"log", log_main},
        {"restore", restore_main},
    };

    if (XBOX_ismatch(&parser, "help")) {
        XBOX_argparse_info(&parser);
        return EXIT_SUCCESS;
    }

    if (cmd) {
        for (int i = 0; i < sizeof(git_commands) / sizeof(git_commands[0]); i++) {
            if (strcmp(cmd, git_commands[i].name) == 0) {
                return git_commands[i].fp(argc - 1, argv + 1);
            }
        }
    } else {
        if (XBOX_ismatch(&parser, "version")) {
            printf("%s\n", VERSION);
            return EXIT_SUCCESS;
        } else {
            XBOX_argparse_info(&parser);
            return EXIT_SUCCESS;
        }
    }

    XBOX_free_argparse(&parser);
    return EXIT_SUCCESS;
}