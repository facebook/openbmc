#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-bic-cached
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Cachcing for Bridge IC info
### END INIT INFO

echo "Setup Caching for Bridge IC info.."
/usr/bin/bic-cached > /dev/null 2>&1 &

echo "done."
