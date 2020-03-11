#!/bin/sh

sudo brctl delif bmc-br0 tap0
sudo ip link set tap0 down
sudo ip tuntap del tap0 mode tap
sudo ip link set dev bmc-br0 down
sudo ip link del dev bmc-br0 type bridge
