#!/bin/bash

# 创建测试文件
touch testfile.txt
if [ ! -f testfile.txt ]; then
    echo "Failed to create test file"
    exit -1
fi

# 修改文件的访问时间和修改时间
touch -t 202301010101 testfile.txt
if [ "$(stat -c %y testfile.txt | cut -d'.' -f1)" == "2023-01-01 01:01:00" ]; then
    echo "utimes success"
else
    echo "utimes failed"
    exit -1
fi

# 清理
rm testfile.txt