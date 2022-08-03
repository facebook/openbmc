#!/bin/sh
# execute <fscd.py> <min syslog level>
# log_level = <debug, warning, info>
# Default log level in fscd is set to warning, if any other level is needed
# set here
. /usr/local/fbpackages/utils/ast-functions

default_fsc_config="/etc/fsc-config.json"
if [[ -f ${default_fsc_config} ]]; then
  rm ${default_fsc_config}
fi

gp_board_id0=$(gpio_get BOARD_ID0 GPION5)
gp_board_id1=$(gpio_get BOARD_ID1 GPION6)
gp_board_id2=$(gpio_get BOARD_ID2 GPION7)

if [ $gp_board_id0 -eq 1 ] && [ $gp_board_id1 -eq 1 ] && [ $gp_board_id2 -eq 0 ]; then
  ln -s /etc/fsc-config_BRCM.json ${default_fsc_config}
else
  ln -s /etc/fsc-config_MICRON.json ${default_fsc_config}
fi

exec /usr/bin/fscd.py 'warning'
