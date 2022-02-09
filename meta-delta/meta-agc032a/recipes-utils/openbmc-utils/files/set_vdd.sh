#!/bin/sh

usage()
{
  echo "usage :"
  echo "   $0 <voltage in mv>"
  echo "   eg. $0 850"
  echo " limit : max 930 mV"
}


if [ $# -ne 1 ]; then
  usage
  exit 255
fi

if [ $(($1)) -gt 931 ]; then
  usage
  exit 255
fi

i2cset -f -y 1 0x40 0x00 0x00
i2cset -f -y 1 0x40 0x20 0x14    # linear mode  ,, N = 0x12
value=$(( $1 * 1000 / 244 ))
i2cset -f -y 1 0x40 0x21 $value w

if [ $? -eq 0 ]; then
  echo "$1"
  exit 0
else
  echo "setting error"
  exit 254
fi
