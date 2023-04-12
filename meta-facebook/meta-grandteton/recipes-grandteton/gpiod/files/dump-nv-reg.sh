#!/bin/bash


dump_nv_log() {
  echo "Start to dump NVDIA FPGA Register"

  TIME=$(date +"%Y%m%d-%S")
  echo $TIME

  DUMP_DIR='/tmp/nv_reg/output'
  DUMP_REG0B="$DUMP_DIR/reg_0x0b.log"
  DUMP_REG13="$DUMP_DIR/reg_0x13.log"
  DUMP_REG14="$DUMP_DIR/reg_0x14.log"
  LOG_ARCHIVE="/mnt/data/nvlog/${TIME}_nvlog.tar.gz"

  mkdir -p "/mnt/data/nvlog"
  mkdir -p $DUMP_DIR

  /usr/sbin/i2cdump -y 11 0x0b > $DUMP_REG0B 2>&1
  /usr/sbin/i2cdump -y 7  0x13 > $DUMP_REG13 2>&1
  /usr/sbin/i2cdump -y 7  0x14 > $DUMP_REG14 2>&1


  tar -zcvf $LOG_ARCHIVE -C $DUMP_DIR reg_0x0b.log reg_0x13.log reg_0x14.log

  echo high > /tmp/gpionames/NV_DUMP_END/direction
  sleep 1
  echo low > /tmp/gpionames/NV_DUMP_END/direction

  echo "End of to dump NVDIA FPGA Register"
}

dump_nv_log &
