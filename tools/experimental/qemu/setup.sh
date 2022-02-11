#!/bin/sh

sudo ip link add dev bmc-br0 type bridge
sudo ip link set dev bmc-br0 up
sudo ip tuntap add tap0 mode tap
sudo ip link set tap0 up
if hash brctl 2>/dev/null; then
  sudo brctl addif bmc-br0 tap0
else
  sudo ip link set tap0 master bmc-br0
fi
