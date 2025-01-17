#!/bin/bash

program="logger"
folder="build"
file="$folder/$program"

if [[ -d "$folder" && -f "$file" ]]; then
  "$file" "$@"
else
  echo "Error: Either the folder '$folder' or the file '$program' does not exist."
  exit 1
fi
