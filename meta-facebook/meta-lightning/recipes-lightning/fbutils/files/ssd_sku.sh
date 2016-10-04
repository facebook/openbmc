#!/bin/bash

FILE=/tmp/ssd_sku_info
MUX_BASE=0x08
I2C_7_MUX_ADDR=0x71
I2C_8_MUX_ADDR=0x72
I2C_M2_MUX_ADDR=0x73
I2C_M2CARD_AMB_ADDR=0x4c
M2_CHANNEL0=0x01

M2_MUX_ADDR=$(echo $I2C_M2_MUX_ADDR | cut -c3-4)
M2CARD_AMB_ADDR=$(echo $I2C_M2CARD_AMB_ADDR | cut -c3-4)

for (( i=0; i<8; i++ ))
do
        NUM=$(($MUX_BASE + $i))
        MUX=$(printf "0x%x" $NUM)
        i2cset -y 7 $I2C_7_MUX_ADDR $MUX
        i2c_scan7=$(i2cdetect -y 7)
        if([[ $i2c_scan7 == *$M2_MUX_ADDR* ]])
        then
                i2cset -y 7 $I2C_M2_MUX_ADDR $M2_CHANNEL0
                i2c_scan7=$(i2cdetect -y 7)
                if([[ $i2c_scan7 == *$M2CARD_AMB_ADDR* ]])
                then
                        echo -e "M2" > $FILE
                        exit 0
                fi
        fi
done

for (( i=0; i<7; i++ ))
do
MUM=$(($MUX_BASE + $i))
        MUX=$(printf "0x%x" $NUM)
        i2cset -y 8 $I2C_8_MUX_ADDR $MUX
        i2c_scan8=$(i2cdetect -y 8)
        if([[ $i2c_scan8 == *$M2_MUX_ADDR* ]])
        then
                i2cset -y 8 $I2C_M2_MUX_ADDR $M2_CHANNEL0
                i2c_scan8=$(i2cdetect -y 8)
                if([[ $i2c_scan8 == *$M2CARD_AMB_ADDR* ]])
                then
                        echo -e "M2" > $FILE
                        exit 0
                fi
        fi
done

echo -e "U2" > $FILE

exit 0
