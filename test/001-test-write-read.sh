#!/bin/bash

# 写入文件
echo "Hello, world!" > testfile.txt
content=$(cat testfile.txt)
if [ "$content" == "Hello, world!" ]; then
    echo "文件写入和读取成功"
else
    echo "文件写入或读取失败"
    exit -1
fi

# 清理
rm testfile.txt
