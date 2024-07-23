#!/bin/bash

# 创建测试文件
touch testfile.txt
if [ ! -f testfile.txt ]; then
    echo "Failed to create test file"
    exit -1
fi

# 修改文件权限
chmod 755 testfile.txt
if [ $(stat -c "%a" testfile.txt) -eq 755 ]; then
    echo "chmod success"
else
    echo "chmod failed"
    exit -1
fi

# 清理
rm testfile.txt