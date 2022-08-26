#!/bin/bash
PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

VPDB_EVT2_REV_ID="2"

if [ "$(kv get vpdb_rev)" -ge $VPDB_EVT2_REV_ID ]; then
  ln -s /etc/sensors_cfg/gt_evt2.conf /etc/sensors.d/gt_rev.conf
else
  ln -s /etc/sensors_cfg/gt_evt.conf /etc/sensors.d/gt_rev.conf
fi


