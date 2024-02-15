#!/bin/bash

KV_CMD="/usr/bin/kv"
I2C_CMD="/usr/sbin/i2cdump"

DUMP_FILE="/tmp/hgx_dump.log"

dump_mb_cpld_reg() {
  echo "Dump MB CPLD reg:" >> $DUMP_FILE

  is_gtt=`i2cdetect -y 7  | grep -i 13`
  if [ -n "$is_gtt" ]; then
    { $I2C_CMD -y 7 0x13;
      $I2C_CMD -y 7 0x14; } >> $DUMP_FILE 2>&1
  else
    $I2C_CMD -y 7 0x23 >> $DUMP_FILE 2>&1
  fi
}

dump_hgx_fpga_reg() {
  echo "Dump HGX FPGA reg:" > $DUMP_FILE
  declare -A GPU_FPGA_REG_INFO=(
    [0x00,0]="GPU1_INT_OVERT"
    [0x00,1]="GPU2_INT_OVERT"
    [0x00,2]="GPU3_INT_OVERT"
    [0x00,3]="GPU4_INT_OVERT"
    [0x00,4]="GPU5_INT_OVERT"
    [0x00,5]="GPU6_INT_OVERT"
    [0x00,6]="GPU7_INT_OVERT"
    [0x00,7]="GPU8_INT_OVERT"

    [0x01,0]="NVSW1_OVERT_INI_N"
    [0x01,1]="NVSW2_OVERT_INI_N"
    [0x01,2]="NVSW3_OVERT_INI_N"
    [0x01,3]="NVSW4_OVERT_INI_N"

    [0x02,0]="GPU1_PWRGD_INI"
    [0x02,1]="GPU2_PWRGD_INI"
    [0x02,2]="GPU3_PWRGD_INI"
    [0x02,3]="GPU4_PWRGD_INI"
    [0x02,4]="GPU5_PWRGD_INI"
    [0x02,5]="GPU6_PWRGD_INI"
    [0x02,6]="GPU7_PWRGD_INI"
    [0x02,7]="GPU8_PWRGD_INI"

    [0x04,0]="NVSW_1_VDD_INT_PG"
    [0x04,1]="NVSW_1_DVDD_INT_PG"
    [0x04,2]="NVSW_1_HVDD_INT_PG"
    [0x04,3]="NVSW_2_VDD_INT_PG"
    [0x04,4]="NVSW_2_DVDD_INT_PG"
    [0x04,5]="NVSW_2_HVDD_INT_PG"
    [0x04,6]="NVSW_12_VDD1V8_INT_PG0"
    [0x04,7]="NVSW_34_VDD1V8_INT_PG0"

    [0x05,0]="NVSW_34_IBC_INT_PG"
    [0x05,1]="NVSW_12_IBC_INT_PG"
    [0x05,2]="NVSW_4_HVDD_INT_PG"
    [0x05,3]="NVSW_4_DVDD_INT_PG"
    [0x05,4]="NVSW_4_VDD_INT_PG"
    [0x05,5]="NVSW_3_HVDD_INT_PG"
    [0x05,6]="NVSW_3_DVDD_INT_PG"
    [0x05,7]="NVSW_3_VDD_INT_PG"

    [0x07,4]="NVSW_12_5V_INT_PG"
    [0x07,5]="NVSW_34_5V_INT_PG"
    [0x07,6]="NVSW_12_3V3_INT_PG"
    [0x07,7]="NVSW_34_3V3_INT_PG"

    [0x0C,1]="FPGA_RET_0123_REFCLK1_DET_INT0"
    [0x0C,2]="FPGA_RET_4567_REFCLK2_DET_INT0"
    [0x0C,3]="FPGA_PEX_REFCLK3_DET_INT0"

    [0x31,3]="FPGA_MIDP_PEX_RST_L[0]"
    [0x31,4]="FPGA_MIDP_PEX_RST_L[1]"
    [0x31,5]="FPGA_MIDP_PEX_RST_L[2]"

    [0x68,0]="RET0123_OVERT_INT_N"
    [0x68,1]="RET4567_OVERT_INT_N"
    [0x68,4]="PEX_SW_OVERT_INT_N"
    [0x68,5]="HMC_OVERT_INT_N"
  )

  offset_last="na"
  for key in "${!GPU_FPGA_REG_INFO[@]}"; do
    offset="${key%,*}"
    if [ "$offset" != "$offset_last" ]; then
      echo "" >> $DUMP_FILE
      echo "i2cget -y -f 11 0x0b "$offset"" >> $DUMP_FILE
      value=$(i2cget -y -f 11 0x0b "$offset")
      if [ $? -ne 0 ]; then
        echo "Fail to get offset: "$offset"" >> $DUMP_FILE
        continue
      fi
      echo "$value" >> $DUMP_FILE
    fi

    if [ "$offset" == "0x31" ]; then
      expected_value=1
    else
      expected_value=0
    fi

    shift_bit="${key#*,}"
    bit_value=$(( (value >> shift_bit) & 0x1 ))

    if [ "$bit_value" != "$expected_value" ]; then
      /usr/bin/logger -t "gpiod" -p daemon.crit "HGX FPGA dump - ${GPU_FPGA_REG_INFO[$key]} Assert"
      echo "HGX FPGA dump: ${GPU_FPGA_REG_INFO[$key]} Assert" >> $DUMP_FILE
    fi

    offset_last="$offset"
  done
}

dump_swb_cpld_reg() {
  echo "Dump SWB CPLD reg:" >> $DUMP_FILE
  $I2C_CMD -y 32 0x13 >> $DUMP_FILE 2>&1
}

dump_log() {
  echo "Start to dump NVDIA FPGA Register"

  TIME=$(date +"%Y%m%d-%H:%M:%S")
  LOG_DIR="/mnt/data/nvlog"
  LOG_ARCHIVE="$LOG_DIR/${TIME}_nvlog.tar.gz"

  dump_hgx_fpga_reg
  echo >> $DUMP_FILE
  dump_mb_cpld_reg
  echo >> $DUMP_FILE
  dump_swb_cpld_reg

  if [ ! -d "$LOG_DIR" ]; then
    mkdir -p "$LOG_DIR"
  fi

  tar -zcvf "$LOG_ARCHIVE" -C "$(dirname "$DUMP_FILE")" "$(basename "$DUMP_FILE")"
  logger -t "gpiod" -p daemon.crit "HGX FPGA dump is generated at $LOG_ARCHIVE"

  echo high > /tmp/gpionames/NV_DUMP_END/direction
  sleep 1
  echo low > /tmp/gpionames/NV_DUMP_END/direction


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
