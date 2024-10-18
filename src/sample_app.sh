#!/bin/bash


single_file=""
multiple_files=""
clients_file=""
state="SYNC"
n_sms=0
sms_size=0


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
    --clients)
      clients_file="$2"
      shift 2
      ;;
    --state)
      state="$2"
      shift 2
      ;;
    --n_sms)
      qos="$2"
      shift 2
      ;;
    --sms_size)
      qos="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done


if [ -n "$single_file" ] && ([ -n "$multiple_files" ] || [ -n "$clients_file" ]); then
  echo "You can only specify one of --file, --files, or --clients"
  exit 1
elif [ -n "$multiple_files" ] && [ -n "$clients_file" ]; then
  echo "You can only specify one of --file, --files, or --clients"
  exit 1
fi

if [ -n "$single_file" ]; then
  ./tinyfile_service.o --n_sms $n_sms --sms_size $sms_size
  ./tinyfile_client.o $state $single_file &
elif [ -n "$multiple_files" ]; then
  ./tinyfile_service.o --n_sms $n_sms --sms_size $sms_size &
  while IFS= read -r line || [ -n "$line" ]; do
    ./tinyfile_client.o $state $line &
  done < "$multiple_files"
elif [ -n "$clients_file" ]; then
  ./tinyfile_service.o --n_sms $n_sms --sms_size $sms_size &
  while IFS= read -r line || [ -n "$line" ]; do
     ./tinyfile_client.o $state $line &
  done < "$clients_file"
else
  echo "You must specify either --file, --files, or --clients"
  exit 1
fi
