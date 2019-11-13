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

function init_class2_sdr() {
  /usr/local/bin/bic-cached 0 > /dev/null 2>&1 &
  /usr/local/bin/bic-cached -r 0x15 0 > /dev/null 2>&1 &
  /usr/local/bin/bic-cached -r 0x10 0 > /dev/null 2>&1 &
}

function init_class1_sdr() {
  if [[ $(is_server_prsnt 0) == "1" ]]; then
    /usr/local/bin/bic-cached 0 > /dev/null 2>&1 &
    /usr/local/bin/bic-cached -r 0x05 0 > /dev/null 2>&1 &
  fi

  if [[ $(is_server_prsnt 1) == "1" ]]; then
    /usr/local/bin/bic-cached 1 > /dev/null 2>&1 &
    /usr/local/bin/bic-cached -r 0x05 1 > /dev/null 2>&1 &
  fi

  if [[ $(is_server_prsnt 2) == "1" ]]; then
    /usr/local/bin/bic-cached 2 > /dev/null 2>&1 &
    /usr/local/bin/bic-cached -r 0x05 2 > /dev/null 2>&1 &
  fi

  if [[ $(is_server_prsnt 3) == "1" ]]; then
    /usr/local/bin/bic-cached 3 > /dev/null 2>&1 &
    /usr/local/bin/bic-cached -r 0x05 3 > /dev/null 2>&1 &
  fi
}

echo -n "Setup Caching for Bridge IC info.."
bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_sdr
elif [ $bmc_location -eq 14 ]; then
  #The BMC of class1
  init_class1_sdr
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

echo "done."
