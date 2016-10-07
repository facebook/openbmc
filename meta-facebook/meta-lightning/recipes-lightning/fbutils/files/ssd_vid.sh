#!/bin/bash

FILE=/tmp/ssd_vendor
MUX_BASE=0x08
I2C_7_MUX_ADDR=0x71
I2C_8_MUX_ADDR=0x72
I2C_M2_MUX_ADDR=0x73
I2C_NVME_INTF_ADDR=0x6a
I2C_VID_ADDR=0x09
M2_CHANNEL0=0x01

INTEL=0x8680
SEAGATE=0xb11b

M2_MUX_ADDR=$(echo $I2C_M2_MUX_ADDR | cut -c3-4)
M2CARD_AMB_ADDR=$(echo $I2C_M2CARD_AMB_ADDR | cut -c3-4)

for (( i=0; i<7; i++ ))
do
        NUM=$(($MUX_BASE + $i))
        MUX=$(printf "0x%x" $NUM)
        i2cset -y 7 $I2C_7_MUX_ADDR $MUX
        i2c_scan7=$(i2cdetect -y 7)
        if([[ $i2c_scan7 == *$M2_MUX_ADDR* ]])
        then
                i2cset -y 7 $I2C_M2_MUX_ADDR $M2_CHANNEL0
        fi
        VID=$(i2cget -y 7 $I2C_NVME_INTF_ADDR $I2C_VID_ADDR w)
        if([[ $VID == *$INTEL* ]])
        then
            echo -e "intel" > $FILE
               exit 0
        elif([[ $VID == *$SEAGATE* ]])
      then
                echo -e "seagate" > $FILE
                exit 0
        fi
done

for (( i=0; i<8; i++ ))
do
        NUM=$(($MUX_BASE + $i))
        MUX=$(printf "0x%x" $NUM)
        i2cset -y 8 $I2C_8_MUX_ADDR $MUX
        i2c_scan8=$(i2cdetect -y 8)
        if([[ $i2c_scan8 == *$M2_MUX_ADDR* ]])
        then
                i2cset -y 8 $I2C_M2_MUX_ADDR $M2_CHANNEL0
        fi
        VID=$(i2cget -y 8 $I2C_NVME_INTF_ADDR $I2C_VID_ADDR w)
        if([[ $VID == *$INTEL* ]])
        then
            echo -e "intel" > $FILE
               exit 0
        elif([[ $VID == *$SEAGATE* ]])
      then
                echo -e "seagate" > $FILE
                exit 0
        fi
done

echo -e "unknown" > $FILE

exit 0
