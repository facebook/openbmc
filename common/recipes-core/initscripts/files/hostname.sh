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
INTERFACE=eth0

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
  #
  # Increasing the number of retries to 20, as 10 is not enough in some DC.
  # In order to reduce the load to DNS server, we use exponential back-off. 
  # That is, the delay between retry will double each time, after loop #10.
  #
  RETRY_MAX=20
  RETRY_DELAY=15
  DELAY_MAX=3600
  LOOP_NUMBER=0
  while [ $LOOP_NUMBER -lt $RETRY_MAX ]; do
    LOOP_NUMBER=$((LOOP_NUMBER+1))
    if [ $LOOP_NUMBER -gt 10 ]; then
      RETRY_DELAY=$((RETRY_DELAY*2))
      if [ $RETRY_DELAY -gt $DELAY_MAX ]; then
         RETRY_DELAY=$DELAY_MAX
      fi
    fi
    # Wait till DHCP gets its IP addresses.
    sleep $RETRY_DELAY
    # Do a DNS lookup for each IP address on eth0 to find a hostname.
    # If multiple hostnames are returned, the sled one, if present, is preferred.
    FOUND_NAME=""
    for ip in $(ip a s $INTERFACE | sed -nr 's, *inet6? ([0-9a-f:.]+)\/.*,\1,p'); do
      dns_ptr=$(dig +short -x "$ip" | sed -nr '/sled.*-oob/ { p; q }; $ { p }')
      dns_ptr=${dns_ptr%?}
      if [ "${dns_ptr:0:2}" != ";;" ]; then
          FOUND_NAME=$dns_ptr
          break
      fi
	  done

    if [ -n "$FOUND_NAME" ]; then
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
