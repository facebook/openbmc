#!/bin/bash

BIC_UTIL="/usr/bin/bic-util"
SLOT_NAME=$1
CPLDDUMP_FILE="/mnt/data/cplddump_$SLOT_NAME"

MAX_INDEX=7


function state_machine_parse {
DUMP_DATA=$1
INDEX=$2

case $INDEX in
    0)
      echo -n "state[0](bit 0): " >> $CPLDDUMP_FILE
      ;;
    1)
      echo -n "state[1](bit 1): " >> $CPLDDUMP_FILE
      ;;
    2)
      echo -n "state[2](bit 2): " >> $CPLDDUMP_FILE
      ;;
    3)
      echo -n "state[3](bit 3): " >> $CPLDDUMP_FILE
      ;;
    4)
      echo -n "state[4](bit 4): " >> $CPLDDUMP_FILE
      ;;
    5)
      echo -n "QDF_TEMPTRIP_1P8_N(bit 5): " >> $CPLDDUMP_FILE
      ;;
    6)
      echo -n "QDF_PROCHOT_1P8_N(bit 6): " >> $CPLDDUMP_FILE
      ;;
    7)
      echo -n "QDF_CPLD_VREG_S4_SENSE_1P8(bit 7): " >> $CPLDDUMP_FILE
      ;;
esac

local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
echo $output >> $CPLDDUMP_FILE
}

function power_seq_enable_parse_p1 {
DUMP_DATA=$1
INDEX=$2

case $INDEX in
    0)
      echo -n "Reserve(bit 0): " >> $CPLDDUMP_FILE
      ;;
    1)
      echo -n "Reserve(bit 1): " >> $CPLDDUMP_FILE
      ;;
    2)
      echo -n "Reserve(bit 2): " >> $CPLDDUMP_FILE
      ;;
    3)
      echo -n "Reserve(bit 3): " >> $CPLDDUMP_FILE
      ;;
    4)
      echo -n "Reserve(bit 4): " >> $CPLDDUMP_FILE
      ;;
    5)
      echo -n "Reserve(bit 5): " >> $CPLDDUMP_FILE
      ;;
    6)
      echo -n "VREG_5P0_EN(bit 6): " >> $CPLDDUMP_FILE
      ;;
    7)
      echo -n "FM_PS_EN(bit 7): " >> $CPLDDUMP_FILE
      ;;
esac

local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
echo $output >> $CPLDDUMP_FILE
}

function power_seq_enable_parse_p2 {
DUMP_DATA=$1
INDEX=$2

case $INDEX in
    0)
      echo -n "Reserve(bit 0): " >> $CPLDDUMP_FILE
      ;;
    1)
      echo -n "VDD_APC_EN(bit 1): " >> $CPLDDUMP_FILE
      ;;
    2)
      echo -n "PVTT_423_EN(bit 2): " >> $CPLDDUMP_FILE
      ;;
    3)
      echo -n "PVTT_510_EN(bit 3): " >> $CPLDDUMP_FILE
      ;;
    4)
      echo -n "PVPP_423_EN(bit 4): " >> $CPLDDUMP_FILE
      ;;
    5)
      echo -n "PVPP_510_EN(bit 5): " >> $CPLDDUMP_FILE
      ;;
    6)
      echo -n "PVDDQ_423_EN(bit 6): " >> $CPLDDUMP_FILE
      ;;
    7)
      echo -n "PVDDQ_510_EN(bit 7): " >> $CPLDDUMP_FILE
      ;;
esac

local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
echo $output >> $CPLDDUMP_FILE
}

function power_seq_pwrgd_parse_p1 {
DUMP_DATA=$1
INDEX=$2

case $INDEX in
    0)
      echo -n "Reserve(bit 0): " >> $CPLDDUMP_FILE
      ;;
    1)
      echo -n "Reserve(bit 1): " >> $CPLDDUMP_FILE
      ;;
    2)
      echo -n "Reserve(bit 2): " >> $CPLDDUMP_FILE
      ;;
    3)
      echo -n "Reserve(bit 3): " >> $CPLDDUMP_FILE
      ;;
    4)
      echo -n "Reserve(bit 4): " >> $CPLDDUMP_FILE
      ;;
    5)
      echo -n "Reserve(bit 5): " >> $CPLDDUMP_FILE
      ;;
    6)
      echo -n "VREG_5P0_PG(bit 6): " >> $CPLDDUMP_FILE
      ;;
    7)
      echo -n "VREG_3P3_PG(bit 7): " >> $CPLDDUMP_FILE
      ;;
esac

local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
echo $output >> $CPLDDUMP_FILE
}

function power_seq_pwrgd_parse_p2 {
DUMP_DATA=$1
INDEX=$2

case $INDEX in
    0)
      echo -n "VDD_APC_PG(bit 0): " >> $CPLDDUMP_FILE
      ;;
    1)
      echo -n "VDD_CBF_PG(bit 1): " >> $CPLDDUMP_FILE
      ;;
    2)
      echo -n "PVTT_423_PG(bit 2): " >> $CPLDDUMP_FILE
      ;;
    3)
      echo -n "PVTT_510_PG(bit 3): " >> $CPLDDUMP_FILE
      ;;
    4)
      echo -n "PVPP_423_PG(bit 4): " >> $CPLDDUMP_FILE
      ;;
    5)
      echo -n "PVPP_510_PG(bit 5): " >> $CPLDDUMP_FILE
      ;;
    6)
      echo -n "PVDDQ_423_PG(bit 6): " >> $CPLDDUMP_FILE
      ;;
    7)
      echo -n "PVDDQ_510_PG(bit 7): " >> $CPLDDUMP_FILE
      ;;
esac

local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
echo $output >> $CPLDDUMP_FILE
}

function event_parse_p1 {
DUMP_DATA=$1
INDEX=$2

case $INDEX in
    0)
      echo -n "PVDDQ_423_VR_FAULT_N(bit 0): " >> $CPLDDUMP_FILE
      ;;
    1)
      echo -n "PVDDQ_510_VR_FAULT_N(bit 1): " >> $CPLDDUMP_FILE
      ;;
    2)
      echo -n "VDD_CBF_VR_FAULT_N(bit 2): " >> $CPLDDUMP_FILE
      ;;
    3)
      echo -n "VDD_APC_VR_FAULT_N(bit 3): " >> $CPLDDUMP_FILE
      ;;
    4)
      echo -n "DDR5_0_EVENT_LVC3_N_R1(bit 4): " >> $CPLDDUMP_FILE
      ;;
    5)
      echo -n "DDR4_0_EVENT_LVC3_N_R1(bit 5): " >> $CPLDDUMP_FILE
      ;;
    6)
      echo -n "DDR3_0_EVENT_LVC3_N_R1(bit 6): " >> $CPLDDUMP_FILE
      ;;
    7)
      echo -n "DDR2_0_EVENT_LVC3_N_R1(bit 7): " >> $CPLDDUMP_FILE
      ;;
esac

local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
echo $output >> $CPLDDUMP_FILE
}

function event_parse_p2 {
DUMP_DATA=$1
INDEX=$2

case $INDEX in
    0)
      echo -n "Reserve(bit 0): " >> $CPLDDUMP_FILE
      ;;
    1)
      echo -n "Reserve(bit 1): " >> $CPLDDUMP_FILE
      ;;
    2)
      echo -n "QDF_FORCE_PSTATE(bit 2): " >> $CPLDDUMP_FILE
      ;;
    3)
      echo -n "QDF_FORCE_PMIN(bit 3): " >> $CPLDDUMP_FILE
      ;;
    4)
      echo -n "QDF_LIGHT_THROTTLE_1P8(bit 4): " >> $CPLDDUMP_FILE
      ;;
    5)
      echo -n "QDF_THROTTLE_1P8(bit 5): " >> $CPLDDUMP_FILE
      ;;
    6)
      echo -n "QDF_PROCHOT_1P8_N(bit 6): " >> $CPLDDUMP_FILE
      ;;
    7)
      echo -n "QDF_TEMPTRIP_1P8_N(bit 7): " >> $CPLDDUMP_FILE
      ;;
esac

local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
echo $output >> $CPLDDUMP_FILE
}

function soc_event_parse {
DUMP_DATA=$1
INDEX=$2

case $INDEX in
    0)
      echo -n "Reserve(bit 0): " >> $CPLDDUMP_FILE
      ;;
    1)
      echo -n "Reserve(bit 1): " >> $CPLDDUMP_FILE
      ;;
    2)
      echo -n "QDF_SMI_1P8(bit 2): " >> $CPLDDUMP_FILE
      ;;
    3)
      echo -n "QDF_NMI_1P8(bit 3): " >> $CPLDDUMP_FILE
      ;;
    4)
      echo -n "IMC_BOOT_ERROR_1P8(bit 4): " >> $CPLDDUMP_FILE
      ;;
    5)
      echo -n "QDF_RAS_ERROR_2_1P8(bit 5): " >> $CPLDDUMP_FILE
      ;;
    6)
      echo -n "QDF_RAS_ERROR_1_1P8(bit 6): " >> $CPLDDUMP_FILE
      ;;
    7)
      echo -n "QDF_RAS_ERROR_0_1P8(bit 7): " >> $CPLDDUMP_FILE
      ;;
esac

local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
echo $output >> $CPLDDUMP_FILE
}

function cpld_dump_process {
  echo "<<< CPLD register 0x04(State Machine) data >>>" >> $CPLDDUMP_FILE

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x04)

  echo -n "Raw Data: 0x" >> $CPLDDUMP_FILE
  echo $output >> $CPLDDUMP_FILE
   
  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    state_machine_parse $output $index
  done

  echo "" >> $CPLDDUMP_FILE
  echo "<<< CPLD register 0x07(Power SEQ Enable) data >>>" >> $CPLDDUMP_FILE

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x07)

  echo -n "Raw Data: 0x" >> $CPLDDUMP_FILE
  echo $output >> $CPLDDUMP_FILE

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_enable_parse_p1 $output $index
  done

  echo "" >> $CPLDDUMP_FILE
  echo "<<< CPLD register 0x08(Power SEQ Enable) data >>>" >> $CPLDDUMP_FILE

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x08)

  echo -n "Raw Data: 0x" >> $CPLDDUMP_FILE
  echo $output >> $CPLDDUMP_FILE

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_enable_parse_p2 $output $index
  done

  echo "" >> $CPLDDUMP_FILE
  echo "<<< CPLD register 0x0B(Power SEQ PWRGD) data >>>" >> $CPLDDUMP_FILE

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x0b)

  echo -n "Raw Data: 0x" >> $CPLDDUMP_FILE
  echo $output >> $CPLDDUMP_FILE

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_pwrgd_parse_p1 $output $index
  done

  echo "" >> $CPLDDUMP_FILE
  echo "<<< CPLD register 0x0C(Power SEQ PWRGD) data >>>" >> $CPLDDUMP_FILE

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x0c)

  echo -n "Raw Data: 0x" >> $CPLDDUMP_FILE
  echo $output >> $CPLDDUMP_FILE

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_pwrgd_parse_p2 $output $index
  done

  echo "" >> $CPLDDUMP_FILE
  echo "<<< CPLD register 0x0F(DIMM & VR EVENT) data >>>" >> $CPLDDUMP_FILE

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x0f)

  echo -n "Raw Data: 0x" >> $CPLDDUMP_FILE
  echo $output >> $CPLDDUMP_FILE

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    event_parse_p1 $output $index
  done

  echo "" >> $CPLDDUMP_FILE
  echo "<<< CPLD register 0x10 data(SOC EVENT) >>>" >> $CPLDDUMP_FILE

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x10)

  echo -n "Raw Data: 0x" >> $CPLDDUMP_FILE
  echo $output >> $CPLDDUMP_FILE

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    event_parse_p2 $output $index
  done

  echo "" >> $CPLDDUMP_FILE
  echo "<<< CPLD register 0x13(SOC EVENT) data >>>" >> $CPLDDUMP_FILE

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x13)

  echo -n "Raw Data: 0x" >> $CPLDDUMP_FILE
  echo $output >> $CPLDDUMP_FILE

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    soc_event_parse $output $index
  done
}

case $SLOT_NAME in
    slot1)
      SLOT_NUM=1
      ;;
    slot2)
      SLOT_NUM=2
      ;;
    slot3)
      SLOT_NUM=3
      ;;
    slot4)
      SLOT_NUM=4
      ;;
    *)
      N=${0##*/}
      N=${N#[SK]??}
      echo "Usage: $N {slot1|slot2|slot3|slot4}"
      exit 1
      ;;
esac

# File format cplddump<slot_id>.pid (See pal_is_cplddump_ongoing()
# function definition)
PID_FILE="/var/run/cplddump$SLOT_NUM.pid"

# check if auto CPLD dump is already running
if [ -f $PID_FILE ]; then
  echo "Another auto CPLD dump for $SLOT_NAME is running"
  exit 1
else
  touch $PID_FILE
fi

# Set cplddump timestamp
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" $sys_runtime)
echo $((sys_runtime+630)) > /tmp/cache_store/fru${SLOT_NUM}_cplddump

LOG_MSG_PREFIX=""

echo "Auto CPLD Dump for $SLOT_NAME Started"

#HEADER LINE for the dump
now=$(date)
echo "Auto CPLD Dump generated at $now" > $CPLDDUMP_FILE

cpld_dump_process

echo -n "Auto CPLD Dump End at " >> $CPLDDUMP_FILE
date >> $CPLDDUMP_FILE

logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}CPLD dump for FRU: $SLOT_NUM is generated at $CPLDDUMP_FILE"

echo "Auto CPLD Dump for $SLOT_NAME Completed"

# Remove current pid file
rm $PID_FILE

echo "${LOG_MSG_PREFIX}Auto CPLD Dump Stored in $CPLDDUMP_FILE"
exit 0


