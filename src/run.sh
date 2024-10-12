#!/bin/bash

# Define source and output directories
SRC_DIR="../bin/input"
OUT_DIR="../bin/output"

rm -r "$OUT_DIR"

# Create the output directory if it doesn't exist
mkdir -p "$OUT_DIR"

# Iterate through all input files
for file in "$SRC_DIR"/*; do
    # Get the filename without path
    filename=$(basename -- "$file")
    #   echo "${filename%.*}"   --> get filename without extension

    COMPRESSED="$OUT_DIR/compressed_$filename"
    UNCOMPRESSED="$OUT_DIR/uncompressed_$filename"

    ./snappyc_test.o "$file" "$COMPRESSED" "$UNCOMPRESSED"
done
