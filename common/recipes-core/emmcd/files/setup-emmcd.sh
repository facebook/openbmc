#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-emmcd
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start eMMC monitor daemon
### END INIT INFO

echo "Setup eMMC Daemon.."

CACHE_STORE_DIR="/tmp/cache_store"
EXTCSD_CACHE="${CACHE_STORE_DIR}/mmc0_extcsd"
MMC_DEVICE="/dev/mmcblk0"
MMCRAW_CMD="/usr/local/bin/mmcraw"

if [ ! -b "$MMC_DEVICE" ]; then
  echo "$MMC_DEVICE does not exist. Exiting."
  exit 1
fi

if [ ! -d "$CACHE_STORE_DIR" ]; then
  mkdir -p "$CACHE_STORE_DIR" || exit 1
fi

if ! [[ -f $EXTCSD_CACHE ]]; then
  "$MMCRAW_CMD" read-extcsd "$MMC_DEVICE" > "$EXTCSD_CACHE"
fi

# Display eMMC device summary
"$MMCRAW_CMD" show-summary "$MMC_DEVICE"

runsv /etc/sv/emmcd > /dev/null 2>&1 &
