#!/bin/bash

KV_CMD="/usr/bin/kv"
I2C_CMD="/usr/sbin/i2ctransfer"
DUMP_FILE="/tmp/hgx_dump.log"

declare hgx_fpga_reg=""

dump_hgx_fpga_reg() {
  echo "Dump HGX FPGA reg:" > $DUMP_FILE
  hgx_fpga_reg=$(i2ctransfer -y 11 w1@0x0b 0x00 r256)
  echo $hgx_fpga_reg >> $DUMP_FILE 2>&1
}

dump_mb_cpld_reg() {
  echo "Dump MB CPLD reg:" >> $DUMP_FILE

  is_gtt=`i2cdetect -y 7  | grep -i 13`
  if [ -n "$is_gtt" ]; then
    { echo "Offset = 0x13";
      $I2C_CMD -y 7 w1@0x13 0x00 r256;
      echo "Offset = 0x14";
      $I2C_CMD -y 7 w1@0x14 0x00 r256; } >> $DUMP_FILE 2>&1
  else
    $I2C_CMD -y 7 w1@0x23 0x00 r256 >> $DUMP_FILE 2>&1
  fi
}

dump_swb_cpld_reg() {
  echo "Dump SWB CPLD reg:" >> $DUMP_FILE
  $I2C_CMD -y 32 w1@0x13 0x00 r256>> $DUMP_FILE 2>&1
}

parser_hgx_fpga_reg() {
  declare -A GPU_FPGA_REG_INFO=(
  # [Index, Bit#]="SEL_Log"
    [0,0]="GPU1_INT_OVERT"
    [0,1]="GPU2_INT_OVERT"
    [0,2]="GPU3_INT_OVERT"
    [0,3]="GPU4_INT_OVERT"
    [0,4]="GPU5_INT_OVERT"
    [0,5]="GPU6_INT_OVERT"
    [0,6]="GPU7_INT_OVERT"
    [0,7]="GPU8_INT_OVERT"

    [1,0]="NVSW1_OVERT_INI_N"
    [1,1]="NVSW2_OVERT_INI_N"
    [1,2]="NVSW3_OVERT_INI_N"
    [1,3]="NVSW4_OVERT_INI_N"

    [2,0]="GPU1_PWRGD_INI"
    [2,1]="GPU2_PWRGD_INI"
    [2,2]="GPU3_PWRGD_INI"
    [2,3]="GPU4_PWRGD_INI"
    [2,4]="GPU5_PWRGD_INI"
    [2,5]="GPU6_PWRGD_INI"
    [2,6]="GPU7_PWRGD_INI"
    [2,7]="GPU8_PWRGD_INI"

    [4,0]="NVSW_1_VDD_INT_PG"
    [4,1]="NVSW_1_DVDD_INT_PG"
    [4,2]="NVSW_1_HVDD_INT_PG"
    [4,3]="NVSW_2_VDD_INT_PG"
    [4,4]="NVSW_2_DVDD_INT_PG"
    [4,5]="NVSW_2_HVDD_INT_PG"
    [4,6]="NVSW_12_VDD1V8_INT_PG0"
    [4,7]="NVSW_34_VDD1V8_INT_PG0"

    [5,0]="NVSW_34_IBC_INT_PG"
    [5,1]="NVSW_12_IBC_INT_PG"
    [5,2]="NVSW_4_HVDD_INT_PG"
    [5,3]="NVSW_4_DVDD_INT_PG"
    [5,4]="NVSW_4_VDD_INT_PG"
    [5,5]="NVSW_3_HVDD_INT_PG"
    [5,6]="NVSW_3_DVDD_INT_PG"
    [5,7]="NVSW_3_VDD_INT_PG"

    [7,4]="NVSW_12_5V_INT_PG"
    [7,5]="NVSW_34_5V_INT_PG"
    [7,6]="NVSW_12_3V3_INT_PG"
    [7,7]="NVSW_34_3V3_INT_PG"

    [12,1]="FPGA_RET_0123_REFCLK1_DET_INT0"
    [12,2]="FPGA_RET_4567_REFCLK2_DET_INT0"
    [12,3]="FPGA_PEX_REFCLK3_DET_INT0"

    [49,3]="FPGA_MIDP_PEX_RST_L[0]"
    [49,4]="FPGA_MIDP_PEX_RST_L[1]"
    [49,5]="FPGA_MIDP_PEX_RST_L[2]"

    [104,0]="RET0123_OVERT_INT_N"
    [104,1]="RET4567_OVERT_INT_N"
    [104,4]="PEX_SW_OVERT_INT_N"
    [104,5]="HMC_OVERT_INT_N"
  )


  offset_last="na"
  for key in "${!GPU_FPGA_REG_INFO[@]}"; do
    offset="${key%,*}"
    offset=$((offset + 1))
    if [ "$offset" != "$offset_last" ]; then
      val=$(echo "$hgx_fpga_reg" | awk '{print $'"$offset"'}')
    fi

    # When the value of FPGA_MIDP_PEX_RST_L[X] = 1 indicates normal
    if [ "$offset" == "50" ]; then
      expected_value=1
    else
      expected_value=0
    fi

    shift_bit="${key#*,}"
    bit_value=$(( (val >> shift_bit) & 0x1 ))

    if [ "$bit_value" != "$expected_value" ]; then
      /usr/bin/logger -t "gpiod" -p daemon.crit "HGX FPGA dump - ${GPU_FPGA_REG_INFO[$key]} Assert"
      echo "HGX FPGA dump: ${GPU_FPGA_REG_INFO[$key]} Assert" >> "$DUMP_FILE"
    fi

    offset_last="$offset"
  done
}

dump_log() {
  echo "Start to dump NVDIA FPGA Register"

  TIME=$(date +"%Y%m%d-%H:%M:%S")
  LOG_DIR="/mnt/data/nvlog"
  LOG_ARCHIVE="$LOG_DIR/${TIME}_nvlog.tar.gz"

  dump_hgx_fpga_reg
  dump_mb_cpld_reg
  dump_swb_cpld_reg

  echo high > /tmp/gpionames/NV_DUMP_END/direction
  sleep 1
  echo low > /tmp/gpionames/NV_DUMP_END/direction

  parser_hgx_fpga_reg

  if [ ! -d "$LOG_DIR" ]; then
    mkdir -p "$LOG_DIR"
  fi

  tar -zcvf "$LOG_ARCHIVE" -C "$(dirname "$DUMP_FILE")" "$(basename "$DUMP_FILE")"
  logger -t "gpiod" -p daemon.crit "HGX FPGA dump is generated at $LOG_ARCHIVE"

  echo "End of to dump NVDIA FPGA Register"
}

state=`$KV_CMD get "hgx_dump_stat"`
if [ "$state" == "Ongoing" ]; then
  exit 0
else
  $KV_CMD set "hgx_dump_stat" "Ongoing"
  dump_log
  $KV_CMD set "hgx_dump_stat" "Done"
fi
