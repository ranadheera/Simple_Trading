#!/bin/bash

output_file="all_source_files.txt"
> "$output_file"  # Clear or create output file

# Find .cpp and .h files recursively in current directory
find . -type f \( -name "*.cpp" -o -name "*.h" \) | sort | while read -r file; do
    echo -e "\n=== BEGIN $file ===" >> "$output_file"
    cat "$file" >> "$output_file"
    echo -e "\n=== END $file ===" >> "$output_file"
done

echo "All files have been dumped into $output_file"
