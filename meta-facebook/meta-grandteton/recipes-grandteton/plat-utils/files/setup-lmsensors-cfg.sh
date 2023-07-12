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


VPDB_HSC_RSENSE_1ST=0
VPDB_HSC_RSENSE_2ND=1

#VPDB ADM1272 config
for i in {1..5}
do
  rev=$(fruid-util vpdb 2>/dev/null |grep "Board Part Number" | awk -F ":" '{print $2}' | awk '{gsub(/^ +| +$/,"")}1')
  rc=$?

  if [ "$rc" == 1 ] || [ -z "$rev" ]; then
    sleep 1
  elif [ "$rev" == "35F0TPB0000" ]; then
    ln -s /etc/sensors_cfg/vpdb-adm1272-1.conf /etc/sensors.d/vpdb_adm1272.conf
    kv set vpdb_rsense_source "$VPDB_HSC_RSENSE_1ST"
    break
  elif [ "$rev" == "35F0TPB0020" ] || [ "$rev" == "35F0TPB0040" ]; then
    ln -s /etc/sensors_cfg/vpdb-adm1272-2.conf /etc/sensors.d/vpdb_adm1272.conf
    kv set vpdb_rsense_source "$VPDB_HSC_RSENSE_2ND"
    break
  fi

  if [ $i -eq 5 ]; then
    ln -s /etc/sensors_cfg/vpdb-adm1272-1.conf /etc/sensors.d/vpdb_adm1272.conf
    kv set vpdb_rsense_source "$VPDB_HSC_RSENSE_1ST"
  fi
done

HPDB_HSC_RSENSE_1ST=0
HPDB_HSC_RSENSE_2ND=1
#HPDB ADM1272 config
for i in {1..5}
  do
  rev=$(fruid-util hpdb 2>/dev/null |grep "Board Part Number" | awk -F ":" '{print $2}' | awk '{gsub(/^ +| +$/,"")}1')
  rc=$?

  if [ "$rc" == 1 ] || [ -z "$rev" ]; then
    sleep 1
  elif [ "$rev" == "35F0TPB0060" ]; then
    ln -s /etc/sensors_cfg/hpdb-adm1272-2.conf /etc/sensors.d/hpdb_adm1272.conf
    kv set hpdb_rsense_source "$HPDB_HSC_RSENSE_2ND"
    break
  elif [ "$rev" == "35F0TPB0010" ] || [ "$rev" == "35F0TPB0090" ]; then
    ln -s /etc/sensors_cfg/hpdb-adm1272-1.conf /etc/sensors.d/hpdb_adm1272.conf
    kv set hpdb_rsense_source "$HPDB_HSC_RSENSE_1ST"
    break
  fi

  if [ $i -eq 5 ]; then
    ln -s /etc/sensors_cfg/hpdb-adm1272-1.conf /etc/sensors.d/hpdb_adm1272.conf
    kv set hpdb_rsense_source "$HPDB_HSC_RSENSE_1ST"
  fi
done
