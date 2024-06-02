#!/bin/bash

# 创建目录
mkdir testdir

# 在目录中创建文件
touch testdir/testfile.txt
if [ -f testdir/testfile.txt ]; then
    echo "在目录中创建文件成功"
else
    echo "在目录中创建文件失败"
fi

# 清理
rm testdir/testfile.txt
rmdir testdir
