#!/bin/bash

folder="p_launcher/build"
file="$folder/p_t"

if [[ -d "$folder" && -f "$file" ]]; then
    "$file" "$@"
else
    echo "Error: Either the folder '$folder' or the file 'p_t' does not exist."
    exit 1
fi
