#!/bin/sh

sudo ip link add dev bmc-br0 type bridge
sudo ip link set dev bmc-br0 up
sudo ip tuntap add tap0 mode tap
sudo ip link set tap0 up
sudo brctl addif bmc-br0 tap0
