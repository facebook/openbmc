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

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function power_seq_enable_parse_p1 {
  DUMP_DATA=$1
  INDEX=$2

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
      echo -n "Reserve(bit 4): "
      ;;
    5)
      echo -n "Reserve(bit 5): "
      ;;
    6)
      echo -n "VREG_5P0_EN(bit 6): "
      ;;
    7)
      echo -n "FM_PS_EN(bit 7): "
      ;;
  esac

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function power_seq_enable_parse_p2 {
  DUMP_DATA=$1
  INDEX=$2

  case $INDEX in
    0)
      echo -n "Reserve(bit 0): "
      ;;
    1)
      echo -n "VDD_APC_EN(bit 1): "
      ;;
    2)
      echo -n "PVTT_423_EN(bit 2): "
      ;;
    3)
      echo -n "PVTT_510_EN(bit 3): "
      ;;
    4)
      echo -n "PVPP_423_EN(bit 4): "
      ;;
    5)
      echo -n "PVPP_510_EN(bit 5): "
      ;;
    6)
      echo -n "PVDDQ_423_EN(bit 6): "
      ;;
    7)
      echo -n "PVDDQ_510_EN(bit 7): "
      ;;
  esac

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function power_seq_pwrgd_parse_p1 {
  DUMP_DATA=$1
  INDEX=$2

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
      echo -n "Reserve(bit 4): "
      ;;
    5)
      echo -n "Reserve(bit 5): "
      ;;
    6)
      echo -n "VREG_5P0_PG(bit 6): "
      ;;
    7)
      echo -n "VREG_3P3_PG(bit 7): "
      ;;
  esac

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function power_seq_pwrgd_parse_p2 {
  DUMP_DATA=$1
  INDEX=$2

  case $INDEX in
    0)
      echo -n "VDD_APC_PG(bit 0): "
      ;;
    1)
      echo -n "VDD_CBF_PG(bit 1): "
      ;;
    2)
      echo -n "PVTT_423_PG(bit 2): "
      ;;
    3)
      echo -n "PVTT_510_PG(bit 3): "
      ;;
    4)
      echo -n "PVPP_423_PG(bit 4): "
      ;;
    5)
      echo -n "PVPP_510_PG(bit 5): "
      ;;
    6)
      echo -n "PVDDQ_423_PG(bit 6): "
      ;;
    7)
      echo -n "PVDDQ_510_PG(bit 7): "
      ;;
  esac

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function event_parse_p1 {
  DUMP_DATA=$1
  INDEX=$2

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

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function event_parse_p2 {
  DUMP_DATA=$1
  INDEX=$2

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

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function soc_event_parse {
  DUMP_DATA=$1
  INDEX=$2

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

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function cpld_dump_rc {
  echo "<<< CPLD register 0x04(State Machine) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x04)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    state_machine_parse $output $index
  done

  echo ""
  echo "<<< CPLD register 0x07(Power SEQ Enable) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x07)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_enable_parse_p1 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x08(Power SEQ Enable) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x08)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_enable_parse_p2 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x0B(Power SEQ PWRGD) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x0b)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_pwrgd_parse_p1 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x0C(Power SEQ PWRGD) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x0c)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    power_seq_pwrgd_parse_p2 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x0F(DIMM & VR EVENT) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x0f)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    event_parse_p1 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x10 data(SOC EVENT) >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x10)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    event_parse_p2 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x13(SOC EVENT) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x01 0x1e 0x01 0x13)

  echo -n "Raw Data: 0x"
  echo $output

  for (( index=0; index<=MAX_INDEX; index=index+1 ))
  do
    soc_event_parse $output $index
  done
}

cpld_dump_rc

exit 0
