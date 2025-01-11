#!/bin/bash

folder="logger/build"
file="$folder/logger"

if [[ -d "$folder" && -f "$file" ]]; then
    "$file" "$@"
else
    echo "Error: Either the folder '$folder' or the file 'logger' does not exist."
    exit 1
fi
