#!/bin/bash

connect_uart2_4() {
  echo -ne "uart2" > /sys/devices/platform/ahb/ahb:apb/1e789000.lpc/1e789098.uart_routing/uart4
  echo -ne "uart4" > /sys/devices/platform/ahb/ahb:apb/1e789000.lpc/1e789098.uart_routing/uart2
}

connect_uart2_4
