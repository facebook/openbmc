#!/bin/bash
export PATH=$PATH:/usr/local/bin

# Define the source and destination directories
SOURCE_DIR="$HOME/local/openbmc/common/recipes-utils/fixmybmc/files"
DESTINATION_DIR="/usr/lib/python*/site-packages/"
# Get the hostname from the command line argument
HOSTNAME=$1
# Check if the hostname is provided
if [ -z "$HOSTNAME" ]; then
  echo "Please provide the hostname as an argument."
  exit 1
fi

scp -r "$SOURCE_DIR/fixmybmc/" "root@$HOSTNAME:$DESTINATION_DIR"
scp -p "$SOURCE_DIR/fixmybmc.sh" "root@$HOSTNAME:/usr/bin/fixmybmc"
