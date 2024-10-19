#!/bin/bash


single_file=""
multiple_files=""
state="SYNC"
n_sms=0
sms_size=0

make clean
make

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
    --n_sms)
      n_sms="$2"
      shift 2
      ;;    
    --sms_size)
      sms_size="$2"
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
# echo "$n_sms"
# echo "$sms_size"

if [ -n "$single_file" ]; then
  ./tinyfile_service.o --n_sms "$n_sms" --sms_size "$sms_size" &
  ./tinyfile_client.o $state $single_file &
elif [ -n "$multiple_files" ]; then
  ./tinyfile_service.o --n_sms "$n_sms" --sms_size "$sms_size" &
  while IFS= read -r line || [ -n "$line" ]; do
    ./tinyfile_client.o $state $line &
  done < "$multiple_files"
else
  echo "You must specify either --file, --files, or --clients"
  exit 1
fi