#!/bin/bash

BASE_DIR=/tmp/overlay

help() {
  echo "Helper to overlay a writable FS for a RO partition"
  echo "USAGE $0 /path/to/target [Optional BASE_DIR, default: $BASE_DIR]"
  echo "Example: $0 /usr"
}

if [ "$1" == "" ]; then
  help
  exit 1
fi

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
  help
  exit 0
fi
TARGET=$1
if [ "$2" != "" ]; then
  if [ ! -d "$2" ]; then
    echo "$2 [BASE DIR] does not exist"
    exit 1
  fi
  BASE_DIR=$2
fi

if [ ! -d "$TARGET" ]; then
  echo "Directory $TARGET does not exist"
  exit 1
fi

UPPER=$BASE_DIR/upper${TARGET}
WORK=$BASE_DIR/work${TARGET}
current=$(mount | grep " ${TARGET} " | grep "upperdir=${UPPER}")
if [ "$current" != "" ]; then
  echo "$TARGET already has an overlay to ${UPPER}!"
  exit 1
fi

mkdir -p "$UPPER"
mkdir -p "$WORK"
mount -t overlay -o rw,lowerdir="${TARGET}",upperdir="${UPPER}",workdir="${WORK}" overlay "${TARGET}"
