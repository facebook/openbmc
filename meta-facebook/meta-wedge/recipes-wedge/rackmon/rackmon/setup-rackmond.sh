#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-rackmond
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start Rackmon service
### END INIT INFO

echo -n "Starting rackmon background service..."
/usr/local/bin/rackmond
echo "done."

echo -n "Configuring rackmon service..."
python /etc/rackmon-config.py
echo "done."
