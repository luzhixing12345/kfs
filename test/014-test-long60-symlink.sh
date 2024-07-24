#!/bin/bash

# Set the test directory
TEST_DIR="./test_dir"

# File name longer than 60 characters
LONG_FILE_NAME="this_is_a_very_long_file_name_used_for_testing_symbolic_links_creation_exceeding_sixty_characters.txt"

# Symbolic link name
SYMLINK_NAME="long_file_symlink.txt"

# Create the test directory
mkdir -p $TEST_DIR

# Create a file with a long name
echo "Creating a file with a long name..."
dd if=/dev/zero of="$TEST_DIR/$LONG_FILE_NAME" bs=1K count=1
if [[ ! -f "$TEST_DIR/$LONG_FILE_NAME" ]]; then
  echo "File creation failed: $TEST_DIR/$LONG_FILE_NAME"
  exit 1
fi
echo "File with a long name created."

# Create a symbolic link to the long-named file
echo "Creating a symbolic link..."
ln -s "$TEST_DIR/$LONG_FILE_NAME" "$TEST_DIR/$SYMLINK_NAME"
if [[ ! -L "$TEST_DIR/$SYMLINK_NAME" ]]; then
  echo "Symbolic link creation failed: $TEST_DIR/$SYMLINK_NAME"
  exit 1
fi
echo "Symbolic link created."

# Check if the symbolic link points to the correct file
echo "Checking the symbolic link..."
if [[ "$(readlink "$TEST_DIR/$SYMLINK_NAME")" != "$TEST_DIR/$LONG_FILE_NAME" ]]; then
  echo "Symbolic link does not point to the correct file: $TEST_DIR/$SYMLINK_NAME"
  exit 1
fi
echo "Symbolic link check completed."

# Clean up - Delete the symbolic link and the file
echo "Deleting the symbolic link and the file..."
rm "$TEST_DIR/$SYMLINK_NAME"
rm "$TEST_DIR/$LONG_FILE_NAME"

# Check if the symbolic link and file are deleted
if [[ -L "$TEST_DIR/$SYMLINK_NAME" ]] || [[ -f "$TEST_DIR/$LONG_FILE_NAME" ]]; then
  echo "Cleanup failed: symbolic link or file not deleted."
  exit 1
fi
echo "Cleanup completed."

# Remove the test directory
rmdir $TEST_DIR

echo "Test completed."
