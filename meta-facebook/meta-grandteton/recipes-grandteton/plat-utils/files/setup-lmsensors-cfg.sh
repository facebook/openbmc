#!/bin/bash
PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

VPDB_EVT2_REV_ID="2"

if [ "$(kv get vpdb_rev)" -ge $VPDB_EVT2_REV_ID ]; then
  ln -s /etc/sensors_cfg/evt2.conf /etc/sensors.d/rev.conf
else
  ln -s /etc/sensors_cfg/evt.conf /etc/sensors.d/rev.conf
fi

HPDB_ADM1272_1="0"
if [ "$(gpio_get HPDB_SKU_ID_0)" -eq $HPDB_ADM1272_1 ]; then
  ln -s /etc/sensors_cfg/adm1272-1.conf /etc/sensors.d/adm1272.conf
else
  ln -s /etc/sensors_cfg/adm1272-2.conf /etc/sensors.d/adm1272.conf
fi

