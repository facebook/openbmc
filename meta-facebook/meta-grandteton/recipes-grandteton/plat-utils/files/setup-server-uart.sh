#!/bin/sh
connect_uart2_3() {
  val=$(/sbin/devmem 0x1e78909c)
  val=$((val & 0xFE07FFFF))
  val=$((val | 0x01A00000))
  /sbin/devmem 0x1e78909c 32 $val
}

connect_uart2_3
