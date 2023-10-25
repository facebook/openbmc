#!/bin/sh

localEid=8

for busnum in {0..7}
do
    mctp link set mctpi2c${busnum} up
    mctp addr add ${localEid} dev mctpi2c${busnum}
done
