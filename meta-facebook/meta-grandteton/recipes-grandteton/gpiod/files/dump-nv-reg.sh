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
  $I2C_CMD -y 11 0x0b >> $DUMP_FILE 2>&1
}

dump_log() {
  echo "Start to dump NVDIA FPGA Register"

  TIME=$(date +"%Y%m%d-%H:%M:%S")
  LOG_DIR="/mnt/data/nvlog"
  LOG_ARCHIVE="$LOG_DIR/${TIME}_nvlog.tar.gz"

  dump_hgx_fpga_reg
  echo >> $DUMP_FILE
  dump_mb_cpld_reg

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
