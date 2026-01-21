#!/bin/bash
#
# Script to compress serverlist.xml and create MD5 checksum
# Usage: ./create_serverlist.sh <serverlist.xml>
#

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <serverlist.xml>"
    echo "Example: $0 serverlist.xml"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="${INPUT_FILE}.z"
MD5_FILE="${OUTPUT_FILE}.md5"

# Check if input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' does not exist."
    exit 2
fi

# Check if zlib_compress binary exists
ZLIB_COMPRESS="./build/bin/zlib_compress"
if [ ! -f "$ZLIB_COMPRESS" ]; then
    echo "Error: zlib_compress binary not found at '$ZLIB_COMPRESS'"
    echo "Please build it first with: cmake --build ./build --target zlib_compress"
    exit 3
fi

echo "Compressing '$INPUT_FILE' to '$OUTPUT_FILE'..."
"$ZLIB_COMPRESS" "$INPUT_FILE"

# Check if compression was successful
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Error: Compression failed - output file not created."
    exit 4
fi

echo "Creating MD5 checksum file '$MD5_FILE'..."
md5sum "$OUTPUT_FILE" | awk '{print $1}' > "$MD5_FILE"

echo ""
echo "✓ Success!"
echo "  Compressed file: $OUTPUT_FILE"
echo "  MD5 checksum:    $MD5_FILE ($(cat "$MD5_FILE"))"
echo ""
echo "Upload these files to your webserver:"
echo "  - $OUTPUT_FILE"
echo "  - $MD5_FILE"
