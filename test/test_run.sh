#!/bin/bash

# Get the directory where the script is located
DIR=$(dirname "$(readlink -f "$0")")

# Set colors
GREEN='\033[0;32m'
RED='\033[0;31m'
PURPLE='\033[0;35m'
NC='\033[0m' # No color

# Initialize counters
total=0
success=0
fail=0

# Files to ignore
ignore_files=("012" "013" "014" "015")
# ignore_files=()

# List all files in the directory that match the pattern and sort by number
files=$(ls -1 "$DIR" | grep -E '^[0-9]{3}-.*\.sh$' | sort)

# Calculate the total number of files
file_count=$(echo "$files" | wc -l)

# Calculate the width for displaying the total number
width=${#file_count}

# Function to check if a file should be ignored
should_ignore() {
    for ignore in "${ignore_files[@]}"; do
        if [[ $1 == $ignore* ]]; then
            printf "[${PURPLE}skip${NC}] %s\n" "$file"
            return 0
        fi
    done
    return 1
}

# Execute each script file
for file in $files; do
    if should_ignore "$file"; then
        continue
    fi
    total=$((total + 1))
    if bash "$DIR/$file" > /dev/null; then
        success=$((success + 1))
        printf "[${GREEN}pass${NC}][%${width}d/%${width}d] %s\n" "$success" "$file_count" "$file"
    else
        fail=$((fail + 1))
        printf "[${RED}fail${NC}][%${width}d/%${width}d] %s\n" "$fail" "$file_count" "$file"
    fi
done

# Output all results
printf "\nTotal: %d\nSuccess: %d\nFail: %d\n" "$total" "$success" "$fail"