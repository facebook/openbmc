#!/bin/bash
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
. /usr/local/bin/openbmc-utils.sh

init_class2_sdr() {
  /usr/local/bin/bic-cached -s slot1
  /usr/local/bin/bic-cached -f slot1 > /dev/null 2>&1 &
}

init_class1_sdr() {
  # -f means `dump fru` only
  if [ "$(is_sb_bic_ready 1)" = "1" ]; then
    i2c_device_add 4 0x50 24c32
    /usr/local/bin/bic-cached -s slot1
    /usr/local/bin/bic-cached -f slot1 > /dev/null 2>&1 &
  fi

  if [ "$(is_sb_bic_ready 2)" = "1" ]; then
    /usr/local/bin/bic-cached -s slot2
    /usr/local/bin/bic-cached -f slot2 > /dev/null 2>&1 &
  fi

  if [ "$(is_sb_bic_ready 3)" = "1" ]; then
    i2c_device_add 6 0x50 24c32
    /usr/local/bin/bic-cached -s slot3
    /usr/local/bin/bic-cached -f slot3 > /dev/null 2>&1 &
  fi

  if [ "$(is_sb_bic_ready 4)" = "1" ]; then
    /usr/local/bin/bic-cached -s slot4
    /usr/local/bin/bic-cached -f slot4 > /dev/null 2>&1 &
  fi
}

echo -n "Setup Caching for Bridge IC info.."
bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_sdr
elif [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_sdr
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

echo "done."
