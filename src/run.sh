#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

single_file=""
multiple_files=""
state="SYNC"


while [ $# -gt 0 ]; do
  case "$1" in
    --file)
      single_file="$2"
      shift 2
      ;;
    --files)
      multiple_files="$2"
      shift 2
      ;;
    --state)
      state="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done


if [ -n "$single_file" ] && ([ -n "$multiple_files" ]); then
  echo "You can only specify one of --file or --files"
  exit 1
fi

# echo "$multiple_files"
# echo "$state"

if [ -n "$single_file" ]; then
  ./tinyfile_client.o $state $single_file &
elif [ -n "$multiple_files" ]; then
  while IFS= read -r line || [ -n "$line" ]; do
    ./tinyfile_client.o $state $line 
  done < "$multiple_files"
else
  echo "You must specify either --file or --files"
  exit 1
fi

