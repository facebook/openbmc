#!/bin/bash
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

