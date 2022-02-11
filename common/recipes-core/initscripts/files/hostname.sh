#!/bin/bash
### BEGIN INIT INFO
# Provides:          hostname
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set hostname based on /etc/hostname
### END INIT INFO
HOSTNAME="bmc-oob."

set_hostname() {
  local name=$1
  # Update /etc configuration and persistent cache.
  echo $name > /etc/hostname
  echo $name > /mnt/data/hostname
  # Set hostname
  hostname $name
  # Reload/Restart any services needing the updated hostname
  /etc/init.d/syslog reload
}

if [ -s /mnt/data/hostname ]; then
  # Use the cached copy to set the hostname
  HOSTNAME=$(cat /mnt/data/hostname)
fi
set_hostname $HOSTNAME
