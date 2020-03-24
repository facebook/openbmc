#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-front-paneld
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start front panel control daemon
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup Front Panel Daemon.."
  #enable GPIOE4 GPIOE6 pass-through 
  devmem_set_bit $(scu_addr 8C) 14
  devmem_set_bit $(scu_addr 8C) 15
  /usr/local/bin/front-paneld > /dev/null 2>&1 &
echo "done."
