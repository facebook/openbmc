#!/bin/sh
# execute <fscd.py> <min syslog level>
# log_level = <debug, warning, info>
# Default log level in fscd is set to warning, if any other level is needed
# set here
. /usr/local/fbpackages/utils/ast-functions

vr_ic=$(kv get VR_IC)

default_fsc_config="/etc/fsc-config.json"
if [[ -f ${default_fsc_config} ]]; then
  rm ${default_fsc_config}
fi

vendor=$(/usr/local/bin/cfg-util asic_mfr)
if [[ "$vendor" == "AMD" && "$vr_ic" == "VI" ]]; then
  ln -s /etc/fsc-config_AMD_vicor.json ${default_fsc_config}
elif [[ "$vendor" == "AMD" && "$vr_ic" == "IF" ]]; then
  ln -s /etc/fsc-config_AMD_infineon.json ${default_fsc_config}
elif [[ "$vendor" == "NVIDIA" && "$vr_ic" == "VI" ]]; then
  ln -s /etc/fsc-config_NVIDIA_vicor.json ${default_fsc_config}
elif [[ "$vendor" == "NVIDIA" && "$vr_ic" == "IF" ]]; then
  ln -s /etc/fsc-config_NVIDIA_infineon.json ${default_fsc_config}
else
  ln -s /etc/fsc-config_NVIDIA_vicor.json ${default_fsc_config}
fi

exec /usr/bin/fscd.py 'warning'
