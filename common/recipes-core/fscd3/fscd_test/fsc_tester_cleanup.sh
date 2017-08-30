#!/bin/sh

rm -f ../fscd/*.pyc
rm -rf ./*.pyc
rm -rf ./ply/*.pyc
rm -f ../fscd/parser.out
rm -f ../fscd/parsetab.py

# reset all fan set data
echo 0 > ./test-data/test-util-data/fan0-set-data
echo 0 > ./test-data/test-util-data/fan1-set-data
echo 0 > ./test-data/test-util-data/fan2-set-data
echo 0 > ./test-data/test-util-data/fan3-set-data
echo 0 > ./test-data/test-util-data/fan4-set-data
echo 0 > ./test-data/test-util-data/fan5-set-data
echo 0 > ./test-data/test-sysfs-data/fantray0_pwm
echo 0 > ./test-data/test-sysfs-data/fantray1_pwm
echo 0 > ./test-data/test-sysfs-data/fantray2_pwm
echo 0 > ./test-data/test-sysfs-data/fantray3_pwm
echo 0 > ./test-data/test-sysfs-data/fantray4_pwm
echo 0 > ./test-data/test-sysfs-data/fantray5_pwm
echo 15000 > ./test-data/test-sysfs-data/4-0033/temp1_input
echo 15000 > ./test-data/test-sysfs-data/3-004b/temp1_input
echo 15000 > ./test-data/test-sysfs-data/3-0048/temp1_input
                        
