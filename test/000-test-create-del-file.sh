#!/bin/bash

# 创建文件
touch testfile.txt
if [ -f testfile.txt ]; then
    echo "文件创建成功"
else
    echo "文件创建失败"
fi

# 删除文件
rm testfile.txt
if [ ! -f testfile.txt ]; then
    echo "文件删除成功"
else
    echo "文件删除失败"
fi
