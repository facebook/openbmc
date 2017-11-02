#!/bin/sh
### BEGIN INIT INFO
# Provides:          hostname
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set hostname based on /etc/hostname
### END INIT INFO
HOSTNAME=$(/bin/hostname)

NAME=""
retries=0
while [ $retries -lt 5 ]; do
  IP=$(ip a s eth0 | sed -nr 's, *inet6 ([0-9a-f:]+)/64.*,\1,p; T; q')
  NAME=$(nslookup $IP | sed -nr '$s,.* ,,p')
  if [ "$NAME" != "$IP" ]; then
    echo $NAME > /etc/hostname
    break
  fi
  retries=$((retries + 1))
  sleep 1
done

hostname -b -F /etc/hostname 2> /dev/null
if [ $? -eq 0 ]; then
	exit
fi

# Busybox hostname doesn't support -b so we need implement it on our own
if [ -f /etc/hostname ];then
	hostname `cat /etc/hostname`
elif [ -z "$HOSTNAME" -o "$HOSTNAME" = "(none)" -o ! -z "`echo $HOSTNAME | sed -n '/^[0-9]*\.[0-9].*/p'`" ] ; then
	hostname localhost
fi
