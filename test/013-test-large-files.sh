#!/bin/bash

# Set the test directory
TEST_DIR="./test_dir"

# Declare an associative array of file sizes
declare -A FILE_SIZES
FILE_SIZES=( ["4KB"]=4K ["5KB"]=5K ["100KB"]=100K ["129KB"]=129K ["500KB"]=500K ["1MB"]=1M ["2MB"]=2M )

# Create the test directory
mkdir -p $TEST_DIR

# Create files of different sizes
echo "Creating files of various sizes..."
for FILE_NAME in "${!FILE_SIZES[@]}"; do
  dd if=/dev/zero of="$TEST_DIR/$FILE_NAME" bs=${FILE_SIZES[$FILE_NAME]} count=1
  if [[ ! -f "$TEST_DIR/$FILE_NAME" ]]; then
    echo "File creation failed: $TEST_DIR/$FILE_NAME"
    exit 1
  fi
done
echo "File creation completed."

# Check if the files exist
echo "Checking the files..."
for FILE_NAME in "${!FILE_SIZES[@]}"; do
  if [[ ! -f "$TEST_DIR/$FILE_NAME" ]]; then
    echo "File does not exist: $TEST_DIR/$FILE_NAME"
    exit 1
  fi
done
echo "File check completed."

# Delete the files
echo "Deleting the files..."
for FILE_NAME in "${!FILE_SIZES[@]}"; do
  rm "$TEST_DIR/$FILE_NAME"
  if [[ -f "$TEST_DIR/$FILE_NAME" ]]; then
    echo "File deletion failed: $TEST_DIR/$FILE_NAME"
    exit 1
  fi
done
echo "File deletion completed."

# Check if the files are deleted
echo "Checking the file deletions..."
for FILE_NAME in "${!FILE_SIZES[@]}"; do
  if [[ -f "$TEST_DIR/$FILE_NAME" ]]; then
    echo "File not deleted: $TEST_DIR/$FILE_NAME"
    exit 1
  fi
done
echo "File deletion check completed."

# Remove the test directory
rmdir $TEST_DIR

echo "Test completed."
