#!/bin/bash

# 获取脚本所在的目录
DIR=$(dirname "$(readlink -f "$0")")

# 设置颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # 无颜色

# 初始化计数器
total=0
success=0
fail=0

# 列出脚本所在目录中所有符合格式的文件名,并按序号排序
files=$(ls -1 "$DIR" | grep -E '^[0-9]{3}-.*\.sh$' | sort)

# 计算总文件数
file_count=$(echo "$files" | wc -l)

# 计算显示总数所需的宽度
width=${#file_count}

# 依次执行每个脚本文件
for file in $files; do
    total=$((total + 1))
    if bash "$DIR/$file" > /dev/null; then
        success=$((success + 1))
        printf "[${GREEN}pass${NC}][%${width}d/%${width}d] %s\n" "$success" "$file_count" "$file"
    else
        fail=$((fail + 1))
        printf "[${RED}fail${NC}][%${width}d/%${width}d] %s\n" "$fail" "$file_count" "$file"
    fi
done

# 统计输出所有结果
printf "\nTotal: %d\nSuccess: %d\nFail: %d\n" "$total" "$success" "$fail"