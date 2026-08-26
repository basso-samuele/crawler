#!/usr/bin/env bash

input_file="$1"

if [[ -z "$input_file" ]]; then
    echo "Usage: $0 <input-file>" >&2
    exit 1
fi

if [[ ! -f "$input_file" ]]; then
    echo "Error: $input_file not found" >$2
    exit 1
fi

output_dir="$(dirname "$input_file")"
output_file="$output_dir/tags.enum"

> "$output_file"

while IFS= read -r tag; do
    [[ -z "$tag" ]] && continue
    upper="${tag^^}"
    echo "CRAWLER_ELEMENT_${upper}," >> "$output_file"
done < "$input_file"