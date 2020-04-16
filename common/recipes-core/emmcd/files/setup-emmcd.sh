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

EXTCSD_CACHE="/tmp/cache_store/mmc0_extcsd"
MMC_DEVICE="/dev/mmcblk0"

if ! [[ -f $EXTCSD_CACHE ]]; then
  /usr/local/bin/mmcraw read-extcsd $MMC_DEVICE > $EXTCSD_CACHE
fi

# BUS_WIDTH (EXTCSD[183])
bus_width=$(grep "176-191" $EXTCSD_CACHE | cut -d " " -f 10)
case "$bus_width" in
  "00")
    bus_width="1"
    ;;
  "01")
    bus_width="4"
    ;;
  "02")
    bus_width="8"
    ;;
  "05")
    bus_width="4 with dual data"
    ;;
  "06")
    bus_width="8 with dual data"
    ;;
  *)
    bus_width="Unknown"
    ;;
esac
echo "eMMC Bus width: $bus_width bit(s)"

# CACHE_CTRL (EXTCSD[33])
cache_ctl=$(grep "32 - 47" $EXTCSD_CACHE | cut -d " " -f 5)
case "$cache_ctl" in
  "00")
    cache_ctl="OFF"
    ;;
  "01")
    cache_ctl="ON"
    ;;
  *)
    cache_ctl="Unknown"
    ;;
esac
echo "eMMC Cache: $cache_ctl"

# RST_n_FUNCTION (EXTCSD[162])
hw_reset=$(grep "160-175" $EXTCSD_CACHE | cut -d " " -f 4)
case "$hw_reset" in
  "00")
    hw_reset="Temporarily disabled"
    ;;
  "01")
    hw_reset="Permanently enabled"
    ;;
  "02")
    hw_reset="Permanently disabled"
    ;;
  *)
    hw_reset="Unknown"
    ;;
esac
echo "eMMC H/W reset: $hw_reset"

runsv /etc/sv/emmcd > /dev/null 2>&1 &

