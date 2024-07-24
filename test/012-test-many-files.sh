#!/bin/bash

# Set the test directory
TEST_DIR="./test_dir"

# File prefix and number of files
FILE_PREFIX="test_file_"
NUM_FILES=1000

# Create the test directory
mkdir -p $TEST_DIR

# Create files
echo "Starting file creation..."
for i in $(seq 1 $NUM_FILES); do
  touch "$TEST_DIR/$FILE_PREFIX$i"
  if [[ ! -f "$TEST_DIR/$FILE_PREFIX$i" ]]; then
    echo "File creation failed: $TEST_DIR/$FILE_PREFIX$i"
    exit 1
  fi
done
echo "File creation completed."

# Check if files exist
echo "Starting file check..."
for i in $(seq 1 $NUM_FILES); do
  if [[ ! -f "$TEST_DIR/$FILE_PREFIX$i" ]]; then
    echo "File does not exist: $TEST_DIR/$FILE_PREFIX$i"
    exit 1
  fi
done
echo "File check completed."

# Delete files
echo "Starting file deletion..."
for i in $(seq 1 $NUM_FILES); do
  rm "$TEST_DIR/$FILE_PREFIX$i"
  if [[ -f "$TEST_DIR/$FILE_PREFIX$i" ]]; then
    echo "File deletion failed: $TEST_DIR/$FILE_PREFIX$i"
    exit 1
  fi
done
echo "File deletion completed."

# Check if files are deleted
echo "Starting file deletion check..."
for i in $(seq 1 $NUM_FILES); do
  if [[ -f "$TEST_DIR/$FILE_PREFIX$i" ]]; then
    echo "File not deleted: $TEST_DIR/$FILE_PREFIX$i"
    exit 1
  fi
done
echo "File deletion check completed."

# Remove the test directory
rmdir $TEST_DIR

echo "Test completed."
