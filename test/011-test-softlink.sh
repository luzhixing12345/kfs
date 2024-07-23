#!/bin/bash

# 创建测试文件
echo "Hello, world!" > testfile.txt
if [ ! -f testfile.txt ]; then
    echo "Failed to create test file"
    exit -1
fi

# 创建软链接
ln -s testfile.txt symlink.txt
if [ ! -L symlink.txt ]; then
    echo "Failed to create symbolic link"
    exit -1
fi

# 检查软链接内容
symlink_content=$(cat symlink.txt)
if [ "$symlink_content" == "Hello, world!" ]; then
    echo "Symbolic link content is correct"
else
    echo "Symbolic link content is incorrect"
    exit -1
fi

# 修改原文件并检查软链接是否更新
echo "Updated content" > testfile.txt
updated_symlink_content=$(cat symlink.txt)
if [ "$updated_symlink_content" == "Updated content" ]; then
    echo "Symbolic link update is correct"
else
    echo "Symbolic link update is incorrect"
    exit -1
fi

# 清理
rm testfile.txt symlink.txt
