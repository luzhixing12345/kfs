#!/bin/bash

# 创建文件并写入内容
echo "Hello, world!" > testfile.txt

# 创建符号链接
ln -s testfile.txt symlink.txt
if [ -L symlink.txt ]; then
    echo "符号链接创建成功"
else
    echo "符号链接创建失败"
    exit -1
fi

# 检查符号链接内容
symlink_content=$(cat symlink.txt)
if [ "$symlink_content" == "Hello, world!" ]; then
    echo "符号链接内容正确"
else
    echo "符号链接内容错误"
    exit -1
fi

# 清理
rm testfile.txt symlink.txt
