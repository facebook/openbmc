#!/bin/sh
# execute <fscd.py> <min syslog level>
# log_level = <debug, warning, info>
# Default log level in fscd is set to warning, if any other level is needed
# set here
default_fsc_config="/etc/fsc-config.json"
if [[ -f ${default_fsc_config} ]]; then
  rm ${default_fsc_config}
fi

vendor=$(/usr/local/bin/cfg-util asic_mfr)
if [[ "$vendor" == "AMD" ]]; then
  ln -s /etc/fsc-config_AMD.json ${default_fsc_config}
elif [[ "$vendor" == "NVIDIA" ]]; then
  ln -s /etc/fsc-config_NVIDIA.json ${default_fsc_config}
else
  ln -s /etc/fsc-config_NVIDIA.json ${default_fsc_config}
fi

exec /usr/bin/fscd.py 'warning'
