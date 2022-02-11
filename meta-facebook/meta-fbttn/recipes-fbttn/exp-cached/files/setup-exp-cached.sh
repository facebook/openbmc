#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-exp-cached
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Cachcing for Bridge IC info
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup Caching for SCC Expander info.."
#To do... get exp heartbeat

if [ $(is_server_prsnt 1) == "1" ]; then
  /usr/local/bin/exp-cached 1 > /dev/null 2>&1 &
fi

echo "done."
