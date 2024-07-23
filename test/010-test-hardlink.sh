#!/bin/bash

# 创建测试文件
echo "Hello, world!" > testfile.txt
if [ ! -f testfile.txt ]; then
    echo "Failed to create test file"
    exit -1
fi

# 创建硬链接
ln testfile.txt hardlink.txt
if [ ! -f hardlink.txt ]; then
    echo "Failed to create hard link"
    exit -1
fi

# 检查硬链接内容
original_content=$(cat testfile.txt)
hardlink_content=$(cat hardlink.txt)
if [ "$original_content" == "$hardlink_content" ]; then
    echo "Hard link content is correct"
else
    echo "Hard link content is incorrect"
    exit -1
fi

# 修改原文件并检查硬链接是否更新
echo "Updated content" > testfile.txt
updated_content=$(cat hardlink.txt)
if [ "$updated_content" == "Updated content" ]; then
    echo "Hard link update is correct"
else
    echo "Hard link update is incorrect"
    exit -1
fi

# 清理
rm testfile.txt hardlink.txt
