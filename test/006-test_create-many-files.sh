#!/bin/bash
rm -rf testdir

# Create a base directory
mkdir testdir
if [ $? -ne 0 ]; then
    echo "Failed to create base directory"
    exit -1
fi

# Create multiple directories and files
for i in {1..10}; do
    mkdir "testdir/dir$i"
    if [ $? -ne 0 ]; then
        echo "Failed to create directory testdir/dir$i"
        exit -1
    fi
    for j in {1..10}; do
        touch "testdir/dir$i/file$j.txt"
        if [ $? -ne 0 ]; then
            echo "Failed to create file testdir/dir$i/file$j.txt"
            exit -1
        fi
    done
done

echo "Created multiple files and directories successfully"
