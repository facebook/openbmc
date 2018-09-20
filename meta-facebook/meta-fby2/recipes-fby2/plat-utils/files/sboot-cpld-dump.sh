#!/bin/bash
. /usr/local/fbpackages/utils/ast-functions

SLOT_NAME=$1
CPLDDUMP_FILE="/mnt/data/slow_boot_cplddump_$SLOT_NAME"
BIC_UTIL="/usr/bin/bic-util"

MAX_INDEX=7

STATE_MACHINE=("VREG_5P0_EN" "VREG_5P0_PG" "FM_PS_EN" "VREG_3P3_PG" "PWR_FAIL_N" 
               "PVPP_510_EN" "PVPP_423_EN" "PVPP_510_PG" "PVPP_423_PG" "PVDDQ_510_EN"
               "PVDDQ_423_EN" "PVDDQ_510_PG" "PVDDQ_423_PG" "PVTT_510_EN" "PVTT_423_EN"
               "PVTT_510_PG" "PVTT_423_PG" "VDD_CBF_PG" "PMF_RES_OUT_1P8_N" "QDF_PS_HOLD_OUT_1P8"
               "QDF_RES_OUT_1P8_N" "VDD_APC_EN_1P8" "VDD_APC_PG"
              )

lock_state=0

normal_state4=0x1F

normal_state5=0x1FF

normal_state6=0x1FFF

normal_state8=0x1FFFF

normal_state9=0x7FFFF

normal_state11=0x1FFFFF

normal_state12=0x7FFFFF

function state_machine_parse {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "state[0](bit 0): "
      ;;
    1)
      echo -n "state[1](bit 1): "
      ;;
    2)
      echo -n "state[2](bit 2): "
      ;;
    3)
      echo -n "state[3](bit 3): "
      ;;
    4)
      echo -n "state[4](bit 4): "
      ;;
    5)
      echo -n "Reserve(bit 5): "
      ;;
    6)
      echo -n "Reserve(bit 6): "
      ;;
    7)
      echo -n "QDF_CPLD_VREG_S4_SENSE_1P8(bit 7): "
      ;;
  esac

  echo $output
}

function cur_state_machine_parse {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "state[0](bit 0): "
      ;;
    1)
      echo -n "state[1](bit 1): "
      ;;
    2)
      echo -n "state[2](bit 2): "
      ;;
    3)
      echo -n "state[3](bit 3): "
      ;;
    4)
      echo -n "state[4](bit 4): "
      ;;
    5)
      echo -n "QDF_TEMPTRIP_1P8_N(bit 5): "
      ;;
    6)
      echo -n "QDF_PROCHOT_1P8_N(bit 6): "
      ;;
    7)
      echo -n "QDF_CPLD_VREG_S4_SENSE_1P8(bit 7): "
      ;;
  esac

  echo $output
}

function power_seq_enable_parse_p1 {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "Reserve(bit 0): "
      ;;
    1)
      echo -n "Reserve(bit 1): "
      ;;
    2)
      echo -n "Reserve(bit 2): "
      ;;
    3)
      echo -n "Reserve(bit 3): "
      ;;
    4)
      echo -n "PWR_FAIL_N(bit 4): "
      lock_state=$[ $lock_state | $output << 4 ]
      ;;
    5)
      echo -n "VDD_APC_EN_1P8(bit 5): "
      lock_state=$[ $lock_state | $output << 21 ]
      ;;
    6)
      echo -n "VREG_5P0_EN(bit 6): "
      lock_state=$[ $lock_state | $output << 0 ]
      ;;
    7)
      echo -n "FM_PS_EN(bit 7): "
      lock_state=$[ $lock_state | $output << 2 ]
      ;;
  esac

  echo $output
}

function power_seq_enable_parse_p2 {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "Reserve(bit 0): "
      ;;
    1)
      echo -n "VDD_APC_EN(bit 1): "
      ;;
    2)
      echo -n "PVTT_423_EN(bit 2): "
      lock_state=$[ $lock_state | $output << 14 ]
      ;;
    3)
      echo -n "PVTT_510_EN(bit 3): "
      lock_state=$[ $lock_state | $output << 13 ]
      ;;
    4)
      echo -n "PVPP_423_EN(bit 4): "
      lock_state=$[ $lock_state | $output << 6 ]
      ;;
    5)
      echo -n "PVPP_510_EN(bit 5): "
      lock_state=$[ $lock_state | $output << 5 ]
      ;;
    6)
      echo -n "PVDDQ_423_EN(bit 6): "
      lock_state=$[ $lock_state | $output << 10 ]
      ;;
    7)
      echo -n "PVDDQ_510_EN(bit 7): "
      lock_state=$[ $lock_state | $output << 9 ]
      ;;
  esac

  echo $output
}

function power_seq_pwrgd_parse_p1 {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "QDF_RES_OUT_1P8_N(bit 0): "
      lock_state=$[ $lock_state | $output << 20 ]
      ;;
    1)
      echo -n "QDF_PS_HOLD_OUT_1P8(bit 1): "
      lock_state=$[ $lock_state | $output << 19 ]
      ;;
    2)
      echo -n "PMF_RES_OUT_1P8_N(bit 2): "
      lock_state=$[ $lock_state | $output << 18 ]
      ;;
    3)
      echo -n "PWRGD_PS_PWROK(bit 3): "
      ;;
    4)
      echo -n "Reserve(bit 4): "
      ;;
    5)
      echo -n "Reserve(bit 5): "
      ;;
    6)
      echo -n "VREG_5P0_PG(bit 6): "
      lock_state=$[ $lock_state | $output << 1 ]
      ;;
    7)
      echo -n "VREG_3P3_PG(bit 7): "
      lock_state=$[ $lock_state | $output << 3 ]
      ;;
  esac

  echo $output
}

function power_seq_pwrgd_parse_p2 {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "VDD_APC_PG(bit 0): "
      lock_state=$[ $lock_state | $output << 22 ]
      ;;
    1)
      echo -n "VDD_CBF_PG(bit 1): "
      lock_state=$[ $lock_state | $output << 17 ]
      ;;
    2)
      echo -n "PVTT_423_PG(bit 2): "
      lock_state=$[ $lock_state | $output << 16 ]
      ;;
    3)
      echo -n "PVTT_510_PG(bit 3): "
      lock_state=$[ $lock_state | $output << 15 ]
      ;;
    4)
      echo -n "PVPP_423_PG(bit 4): "
      lock_state=$[ $lock_state | $output << 8 ]
      ;;
    5)
      echo -n "PVPP_510_PG(bit 5): "
      lock_state=$[ $lock_state | $output << 7 ]
      ;;
    6)
      echo -n "PVDDQ_423_PG(bit 6): "
      lock_state=$[ $lock_state | $output << 12 ]
      ;;
    7)
      echo -n "PVDDQ_510_PG(bit 7): "
      lock_state=$[ $lock_state | $output << 11 ]
      ;;
  esac

  echo $output
}

function event_parse_p1 {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "PVDDQ_423_VR_FAULT_N(bit 0): "
      ;;
    1)
      echo -n "PVDDQ_510_VR_FAULT_N(bit 1): "
      ;;
    2)
      echo -n "VDD_CBF_VR_FAULT_N(bit 2): "
      ;;
    3)
      echo -n "VDD_APC_VR_FAULT_N(bit 3): "
      ;;
    4)
      echo -n "DDR5_0_EVENT_LVC3_N_R1(bit 4): "
      ;;
    5)
      echo -n "DDR4_0_EVENT_LVC3_N_R1(bit 5): "
      ;;
    6)
      echo -n "DDR3_0_EVENT_LVC3_N_R1(bit 6): "
      ;;
    7)
      echo -n "DDR2_0_EVENT_LVC3_N_R1(bit 7): "
      ;;
  esac

  echo $output
}

function event_parse_p2 {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "Reserve(bit 0): "
      ;;
    1)
      echo -n "Reserve(bit 1): "
      ;;
    2)
      echo -n "QDF_FORCE_PSTATE(bit 2): "
      ;;
    3)
      echo -n "QDF_FORCE_PMIN(bit 3): "
      ;;
    4)
      echo -n "QDF_LIGHT_THROTTLE_1P8(bit 4): "
      ;;
    5)
      echo -n "QDF_THROTTLE_1P8(bit 5): "
      ;;
    6)
      echo -n "QDF_PROCHOT_1P8_N(bit 6): "
      ;;
    7)
      echo -n "QDF_TEMPTRIP_1P8_N(bit 7): "
      ;;
  esac

  echo $output
}

function soc_event_parse {
  DUMP_DATA=$1
  INDEX=$2

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]

  case $INDEX in
    0)
      echo -n "Reserve(bit 0): "
      ;;
    1)
      echo -n "Reserve(bit 1): "
      ;;
    2)
      echo -n "QDF_SMI_1P8(bit 2): "
      ;;
    3)
      echo -n "QDF_NMI_1P8(bit 3): "
      ;;
    4)
      echo -n "IMC_BOOT_ERROR_1P8(bit 4): "
      ;;
    5)
      echo -n "QDF_RAS_ERROR_2_1P8(bit 5): "
      ;;
    6)
      echo -n "QDF_RAS_ERROR_1_1P8(bit 6): "
      ;;
    7)
      echo -n "QDF_RAS_ERROR_0_1P8(bit 7): "
      ;;
  esac

  echo $output
}

function cpld_dump_process {
  echo "<<< CPLD register 0x02(State Machine)(Lock State) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x02)

  echo -n "Raw Data: 0x"
  echo $output

  state=$[ 16#$output & $((16#1f)) ]

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    state_machine_parse $output $index
  done

  echo ""
  echo "<<< CPLD register 0x04(State Machine)(Current State) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x04)

  echo -n "Raw Data: 0x"
  echo $output

  cur_state=$[ 16#$output & $((16#1f)) ]

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    cur_state_machine_parse $output $index
  done

  echo ""
  echo "<<< CPLD register 0x08(Power SEQ Enable) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x08)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_enable_parse_p1 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x0B(Power SEQ Enable) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x0b)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_enable_parse_p2 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x0E(Power SEQ PWRGD) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x0e)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_pwrgd_parse_p1 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x11(Power SEQ PWRGD) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x11)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_pwrgd_parse_p2 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x14(DIMM & VR EVENT) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x14)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    event_parse_p1 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x17 data(SOC EVENT) >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x17)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    event_parse_p2 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x1A(SOC EVENT) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x1a)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    soc_event_parse $output $index
  done
}

function fail_pwr_rail_check {
  echo ""
  echo "<<< Summary of CPLD Dump for Slow Boot >>>"

  case $state in
    2)
      failed_pwr_rail=$((lock_state ^ normal_state4))
      ;;
    4)
      failed_pwr_rail=$((lock_state ^ normal_state5))
      ;;
    5)
      failed_pwr_rail=$((lock_state ^ normal_state6))
      ;;
    6)
      failed_pwr_rail=$((lock_state ^ normal_state8)) 
      ;;
    8)
      failed_pwr_rail=$((lock_state ^ normal_state9))
      ;;
    9)
      failed_pwr_rail=$((lock_state ^ normal_state11)) 
      ;;
    11)
      failed_pwr_rail=$((lock_state ^ normal_state12))
      ;;
    12)
      failed_pwr_rail=$((lock_state ^ normal_state12))
      ;;
    *)
      echo "Last Power State: $state(unknown)"
      echo "Fail Reason: Unknown"
      return
      ;;
  esac

  echo "Last Power State: $state"
  echo "Fail Reason:"

  for (( idx=0; idx < ${#STATE_MACHINE[@]}; idx=idx+1 )); do
    local res=$[ $failed_pwr_rail >> $idx & 1 ]
    if [[ $res -eq 1 ]]; then
      echo ${STATE_MACHINE[$idx]}
    fi
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

# File format sbootcplddump<slot_id>.pid (See pal_is_cplddump_ongoing()
# function definition)
PID_FILE="/var/run/sbootcplddump$SLOT_NUM.pid"

# check if auto CPLD dump for slow boot is already running
if [ -f $PID_FILE ]; then
  echo "Another auto CPLD dump for slow boot for $SLOT_NAME is running"
  exit 1
else
  touch $PID_FILE
fi

# Set cplddump timestamp for slow boot
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" $sys_runtime)
echo $((sys_runtime+630)) > /tmp/cache_store/fru${SLOT_NUM}_sboot_cplddump

LOG_MSG_PREFIX=""

echo "Auto CPLD Dump for Slow Boot for $SLOT_NAME Started"

#HEADER LINE for the dump
now=$(date)
echo "Auto CPLD Dump for Slow Boot generated at $now" > $CPLDDUMP_FILE

cpld_dump_process >> $CPLDDUMP_FILE 

fail_pwr_rail_check >> $CPLDDUMP_FILE

echo -n "Auto CPLD Dump for Slow Boot End at " >> $CPLDDUMP_FILE
date >> $CPLDDUMP_FILE

logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}CPLD dump for Slow Boot for FRU: $SLOT_NUM is generated at $CPLDDUMP_FILE"

echo "Auto CPLD Dump for Slow Boot for $SLOT_NAME Completed"

# Remove current pid file
rm $PID_FILE

echo "${LOG_MSG_PREFIX}Auto CPLD Dump for Slow Boot Stored in $CPLDDUMP_FILE"
exit 0
