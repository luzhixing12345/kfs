#!/bin/bash

# 定义生成随机字符串的函数
random_name() {
    local random_string=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c $1)
    echo "$random_string"
}
# 进入一个新的目录
mkdir testdir
cd testdir
# 最大允许的目录名称长度
MAX_NAME_LENGTH=255

# 生成一个超过最大长度的目录名称
too_long_name=$(random_name 256)

# 输出生成的目录名称长度
echo "Generated name length: ${#too_long_name} characters"

# 尝试创建文件并捕获返回码
touch "$too_long_name"
result=$?

# 检查创建目录的返回码是否失败(非零)
if [ $result -ne 0 ]; then
    echo "Test passed: Failed to create directory with too long name as expected."
else
    echo "Test failed: Directory with too long name was created unexpectedly."
    exit 1
fi

# 创建 20 个长度为 255 的文件名
file_num=20
files=()
for i in $(seq 1 $file_num); do
    file=$(random_name 255)
    # 确保文件名不重复
    while [[ " ${files[@]} " =~ " ${file} " ]]; do
        file=$(random_name 255)
    done
    files+=("$file")
done

# 尝试创建 20 个文件并捕获返回码
for file in "${files[@]}"; do
    touch "$file"
    result=$?
    if [ $result -ne 0 ]; then
        echo "Test failed: Failed to create file with name $file"
        exit 1
    fi
done

# check if all files exist
for file in "${files[@]}"; do
    if [ ! -f "$file" ]; then
        echo "Test failed: File $file does not exist"
        exit 1
    fi
done

# list all files number == file_num
if [ $(ls -1 | wc -l) -ne $file_num ]; then
    echo "Test failed: Expected $file_num files, but got $(ls -1 | wc -l)"
    exit 1
fi

cd ..
rm -rf testdir

echo "Test completed."