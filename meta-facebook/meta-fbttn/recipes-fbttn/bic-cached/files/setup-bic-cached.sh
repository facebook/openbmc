#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-bic-cached
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Cachcing for Bridge IC info
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup Caching for Bridge IC info.."
if [ $(is_server_prsnt 1) == "1" ]; then
  /usr/local/bin/bic-cached 1 > /dev/null 2>&1 &
fi

if [ $(is_server_prsnt 2) == "1" ]; then
/usr/local/bin/bic-cached 2 > /dev/null 2>&1 &
fi

if [ $(is_server_prsnt 3) == "1" ]; then
/usr/local/bin/bic-cached 3 > /dev/null 2>&1 &
fi

if [ $(is_server_prsnt 4) == "1" ]; then
/usr/local/bin/bic-cached 4 > /dev/null 2>&1 &
fi

echo "done."
