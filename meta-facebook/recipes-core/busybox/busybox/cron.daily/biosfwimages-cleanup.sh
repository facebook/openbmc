#!/bin/sh

# Config

EXPIRE_SECONDS=$(( 3600 * 3 ))

# Code
set -e

NOW="$(date +%s)"

is_expired() {
	IMAGE_DIRPATH="$1"; shift
	if [ -f "$IMAGE_DIRPATH"/image ]; then
		UPDATE_TS="$(stat -c %Y "$IMAGE_DIRPATH"/image)"
	else
		UPDATE_TS="$(stat -c %Y "$IMAGE_DIRPATH")"
	fi

	if [ "$(( UPDATE_TS + EXPIRE_SECONDS ))" -lt "$NOW" ]; then
		return 0
	else
		return 1
	fi
}

set -f
IFS="
"
DIRS=($(find /tmp/restapi-bios_images -type d -mindepth 2 -maxdepth 2))
set +f

for DIR in "${DIRS[@]}"; do
	if is_expired "$DIR"; then
		rm -f "$DIR"/*
		rmdir "$DIR"
	fi
done
