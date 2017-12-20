#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-me-cached
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Cachcing for ME info
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup Caching for MEinfo.."
/usr/local/bin/me-cached > /dev/null 2>&1 &
echo "done."
