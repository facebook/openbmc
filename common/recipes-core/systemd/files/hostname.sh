#!/bin/bash
HOSTNAME="bmc-oob."

set_hostname() {
  local name=$1
  # Update /etc configuration and persistent cache.
  echo "$name" > /etc/hostname
  echo "$name" > /mnt/data/hostname
  # Set hostname
  hostname "$name"
}

if [ -s /mnt/data/hostname ]; then
  # Use the cached copy to set the hostname
  HOSTNAME="$(cat /mnt/data/hostname)"
fi

set_hostname "$HOSTNAME"
