#!/bin/bash

BIC_UTIL="/usr/bin/bic-util"
SLOT_NAME=$1
CPLDDUMP_FILE="/mnt/data/cplddump_$SLOT_NAME"

MAX_INDEX=7


STATE_MACHINE=("SmIdle" "SmWaitPwrBtn" "SmSlps4N" " SmPsEnable"
               "SmP12vEn" "SmFmAuxSwEn" "SmP5vPsEn" "SmP3v3PsEn"
               "SmPwrUpSeq0" "SmPwrUpSeq1" "SmPwrUpSeq2" "SmPwrUpSeq3"
               "SmCpuPwrGood" "SmCpuTriStrap" "SmCheckVrGood" "SmSysOn"
               "SmPwrDownSeq0" "SmPwrDownSeq1" "SmPwrDownSeq2" "SmPwrDownSeq3"
               "SmSysOff" "SmPwrFault" "SmVrDead"
              )

function state_machine_parse {
  DUMP_DATA=$1
  INDEX=$2

  case $INDEX in
    0)
      echo -n "State[0] (bit 0): "
      ;;
    1)
      echo -n "State[1] (bit 1): "
      ;;
    2)
      echo -n "State[2] (bit 2): "
      ;;
    3)
      echo -n "State[3] (bit 3): "
      ;;
    4)
      echo -n "State[4] (bit 4): "
      ;;
    5)
      echo -n "State[5] (bit 5): "
      ;;
    6)
      echo -n "State[6] (bit 6): "
      ;;
    7)
      echo -n "State[7] (bit 7): "
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
      echo -n "PWRGD_PVPP_BH (bit 0): "
      ;;
    1)
      echo -n "PWRGD_PVPP_AG (bit 1): "
      ;;
    2)
      echo -n "PWRGD_P0V8_VDD (bit 2): "
      ;;
    3)
      echo -n "PWRGD_PVDD_SOC_R (bit 3): "
      ;;
    4)
      echo -n "PWRGD_PVDD_MEM (bit 4): "
      ;;
    5)
      echo -n "PWRGD_PVDD_SRAM (bit 5): "
      ;;
    6)
      echo -n "PWRGD_PVDD_CORE (bit 6): "
      ;;
    7)
      echo -n "PWRGD_P3V3 (bit 7): "
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
      echo -n "Reserve (bit 0): "
      ;;
    1)
      echo -n "Reserve (bit 1): "
      ;;
    2)
      echo -n "PWRGD_P1V8_STBY_R1 (bit 2): "
      ;;
    3)
      echo -n "PWRGD_P3V3_STBY (bit 3): "
      ;;
    4)
      echo -n "PWRGD_P3V3_CPU_R (bit 4): "
      ;;
    5)
      echo -n "PWRGD_PVDDQ_DDR_BH (bit 5): "
      ;;
    6)
      echo -n "PWRGD_PVDDQ_DDR_AG (bit 6): "
      ;;
    7)
      echo -n "PWRGD_P1V8 (bit 7): "
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
      echo -n "PVPP_BH_EN (bit 0): "
      ;;
    1)
      echo -n "PVPP_AG_EN (bit 1): "
      ;;
    2)
      echo -n "FM_P0V8_VDD_EN (bit 2): "
      ;;
    3)
      echo -n "PVDD_SOC_CPU_EN (bit 3): "
      ;;
    4)
      echo -n "FM_VDD_CORE_CPLD_EN (bit 4): "
      ;;
    5)
      echo -n "PVDD_MEM_CPU_EN (bit 5): "
      ;;
    6)
      echo -n "FM_VDD_SRAM_CPLD_EN (bit 6): "
      ;;
    7)
      echo -n "FM_CPLD_P3V3_EN (bit 7): "
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
      echo -n "Reserve (bit 0): "
      ;;
    1)
      echo -n "Reserve (bit 1): "
      ;;
    2)
      echo -n "Reserve (bit 2): "
      ;;
    3)
      echo -n "FM_P1V8_STBY_R_EN (bit 3): "
      ;;
    4)
      echo -n "FM_P3V3_CPU_EN (bit 4): "
      ;;
    5)
      echo -n "FM_PVDDQ_DDR_BH_EN (bit 5): "
      ;;
    6)
      echo -n "FM_PVDDQ_DDR_AG_EN (bit 6): "
      ;;
    7)
      echo -n "FM_P1V8_VDD_EN (bit 7): "
      ;;
  esac

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function power_seq_enable_parse_p3 {
  DUMP_DATA=$1
  INDEX=$2

  case $INDEX in
    0)
      echo -n "Reserve (bit 0): "
      ;;
    1)
      echo -n "Reserve (bit 1): "
      ;;
    2)
      echo -n "Reserve (bit 2): "
      ;;
    3)
      echo -n "Reserve (bit 3): "
      ;;
    4)
      echo -n "Reserve (bit 4): "
      ;;
    5)
      echo -n "Reserve (bit 5): "
      ;;
    6)
      echo -n "RESET_OUT_L (bit 6): "
      ;;
    7)
      echo -n "CORE_RESET_L (bit 7): "
      ;;
  esac

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function power_fault_parse {
  DUMP_DATA=$1
  INDEX=$2

  case $INDEX in
    0)
      echo -n "Pwr_fault_0 (bit 0): "
      ;;
    1)
      echo -n "Pwr_fault_1 (bit 1): "
      ;;
    2)
      echo -n "Pwr_fault_2 (bit 2): "
      ;;
    3)
      echo -n "Pwr_fault_3 (bit 3): "
      ;;
    4)
      echo -n "Pwr_fault_4 (bit 4): "
      ;;
    5)
      echo -n "Pwr_fault_5 (bit 5): "
      ;;
    6)
      echo -n "Pwr_fault_6 (bit 6): "
      ;;
    7)
      echo -n "Pwr_fault_7 (bit 7): "
      ;;
  esac

  local output=$[ 16#$DUMP_DATA >> $INDEX & 1 ]
  echo $output
}

function cpld_dump_ep {
  echo "<<< CPLD register 0x34 (State Machine) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x03 0x12 0x01 0x34)
  sindex=$((16#$output))
  if [ $sindex -lt ${#STATE_MACHINE[@]} ]; then
    sm_str=${STATE_MACHINE[$sindex]}
  else
    sm_str="Unknown"
  fi
  echo "Raw Data: 0x$output($sm_str)"

  for (( index=0; index<=MAX_INDEX; index=index+1 )); do
    state_machine_parse $output $index
  done

  echo ""
  echo "<<< CPLD register 0x35 (Power SEQ PWRGD) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x03 0x12 0x01 0x35)
  echo "Raw Data: 0x$output"

  for (( index=0; index<=MAX_INDEX; index=index+1 )); do
    power_seq_pwrgd_parse_p1 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x36 (Power SEQ PWRGD) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x03 0x12 0x01 0x36)
  echo "Raw Data: 0x$output"

  for (( index=0; index<=MAX_INDEX; index=index+1 )); do
    power_seq_pwrgd_parse_p2 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x37 (Power SEQ Enable) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x03 0x12 0x01 0x37)
  echo "Raw Data: 0x$output"

  for (( index=0; index<=MAX_INDEX; index=index+1 )); do
    power_seq_enable_parse_p1 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x38 (Power SEQ Enable) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x03 0x12 0x01 0x38)
  echo "Raw Data: 0x$output"

  for (( index=0; index<=MAX_INDEX; index=index+1 )); do
    power_seq_enable_parse_p2 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x39 (Power SEQ Enable) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x03 0x12 0x01 0x39)
  echo "Raw Data: 0x$output"

  for (( index=0; index<=MAX_INDEX; index=index+1 )); do
    power_seq_enable_parse_p3 $output $index
  done

  echo ""
  echo "<<< CPLD register 0x3A (Power Fault) data >>>"

  output=$($BIC_UTIL $SLOT_NAME 0x18 0x52 0x03 0x12 0x01 0x3a)
  echo "Raw Data: 0x$output"

  for (( index=0; index<=MAX_INDEX; index=index+1 )); do
    power_fault_parse $output $index
  done
}

cpld_dump_ep

exit 0
