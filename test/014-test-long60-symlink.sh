#!/bin/bash

# File name longer than 60 characters
LONG_FILE_NAME="this_is_a_very_long_file_name_used_for_testing_symbolic_links_creation_exceeding_sixty_characters.txt"

# Symbolic link name
SYMLINK_NAME="long_file_symlink.txt"

# Create a file with a long name
echo "Creating a file with a long name..."
echo "hello world" > $LONG_FILE_NAME
if [[ ! -f "$LONG_FILE_NAME" ]]; then
  echo "File creation failed: $LONG_FILE_NAME"
  exit 1
fi
echo "File with a long name created."

# Create a symbolic link to the long-named file
echo "Creating a symbolic link..."
ln -s "$LONG_FILE_NAME" "$SYMLINK_NAME"
if [[ ! -L "$SYMLINK_NAME" ]]; then
  echo "Symbolic link creation failed: $SYMLINK_NAME"
  exit 1
fi
echo "Symbolic link created."

# Check if the symbolic link points to the correct file
echo "Checking the symbolic link..."
if [[ "$(readlink "$SYMLINK_NAME")" != "$LONG_FILE_NAME" ]]; then
  echo "Symbolic link does not point to the correct file: $SYMLINK_NAME"
  exit 1
fi
echo "Symbolic link check completed."

# Clean up - Delete the symbolic link and the file
echo "Deleting the symbolic link and the file..."
rm "$SYMLINK_NAME"
rm "$LONG_FILE_NAME"

# Check if the symbolic link and file are deleted
if [[ -L "$SYMLINK_NAME" ]] || [[ -f "$LONG_FILE_NAME" ]]; then
  echo "Cleanup failed: symbolic link or file not deleted."
  exit 1
fi
echo "Cleanup completed."
