# Short-Description: Setup dhclient for IPv6
echo -n "Setup dhclient for IPv6... "
runsv /etc/sv/dhc6 > /dev/null 2>&1 &
echo "done."
