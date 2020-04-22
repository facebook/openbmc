#!/bin/bash

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh
SUPCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-0043)
SUP_PWR_ON_SYSFS="${SUPCPLD_SYSFS_DIR}/cpu_control"

echo "=== USERVER PWR STATUS ==="
if ! val=$(head -n 1 2>/dev/null < "${SUP_PWR_ON_SYSFS}"); then
  echo "Error when trying to read $SUP_PWR_ON_SYSFS, exiting..."
  exit 1
fi

if [ -z "$val" ]; then
  echo "cpu_control register is 0"
else
  echo "cpu_control register is 1"
fi
echo "=== BMC SYSTEM TIME ==="
date
echo "=== BMC VERSION ==="
cat /etc/issue
echo "=== BMC UPTIME ==="
uptime
echo "=== FPGA VERSIONS ==="
fpga_ver.sh
echo "=== CHASSIS SERIAL NUMBER ==="
weutil CHASSIS
echo "=== SWITCH CARD SERIAL NUMBER ==="
weutil SCD
echo "=== SUP SERIAL NUMBER ==="
weutil SUP
echo "=== LC1 SERIAL NUMBER ==="
weutil LC1
echo "=== LC2 SERIAL NUMBER ==="
weutil LC2
echo "=== LC3 SERIAL NUMBER ==="
weutil LC3
echo "=== LC4 SERIAL NUMBER ==="
weutil LC4
echo "=== LC5 SERIAL NUMBER ==="
weutil LC5
echo "=== LC6 SERIAL NUMBER ==="
weutil LC6
echo "=== LC7 SERIAL NUMBER ==="
weutil LC7
echo "=== LC8 SERIAL NUMBER ==="
weutil LC8
echo "=== PSU SHOWTECH AT BUS 5 ==="
python /usr/local/bin/psu_show_tech.py 5 0x58 -c dps1900
echo "=== PSU SHOWTECH AT BUS 8 ==="
python /usr/local/bin/psu_show_tech.py 8 0x58 -c dps1900

