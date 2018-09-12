#!/bin/sh
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

update_hostname() {
  # Wait till DHCP gets its IP addresses.
  sleep 15
  # Do a DNS lookup for each IP address on eth0 to find a hostname.
  # If multiple hostnames are returned, the sled one, if present, is preferred.
  FOUND_NAME=""
  for ip in $(ip a s eth0 | sed -nr 's, *inet6? ([0-9a-f:.]+)\/.*,\1,p'); do
    dns_ptr=$(dig +short -x $ip | sed -nr '/sled.*-oob/ { p; q }; $ { p }')
    dns_ptr=${dns_ptr%?}
    if [ "${dns_ptr:0:2}" != ";;" ]; then
        FOUND_NAME=$dns_ptr
        break
    fi
	done

  # dig didn't return anything. Do not update files.
  [[ -z $FOUND_NAME ]] && return 0

  # Only update if the hostname is different from the one set
  # at bootup.
  if [ "$HOSTNAME" != "$FOUND_NAME" ]; then
    set_hostname $FOUND_NAME
    logger "Hostname changed from $HOSTNAME to $FOUND_NAME"
  fi
}

if [ -e /mnt/data/hostname ]; then
  # Use the cached copy to set the hostname
  HOSTNAME=$(cat /mnt/data/hostname)
fi
set_hostname $HOSTNAME
# Start the update process in the background so
# we may update the cached copy if it has changed.
(update_hostname &)&
