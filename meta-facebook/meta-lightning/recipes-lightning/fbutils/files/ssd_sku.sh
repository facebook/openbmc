#!/bin/bash

. /usr/local/fbpackages/utils/ast-functions
. /usr/bin/kv

MUX_BASE=0x08
I2C_7_MUX_ADDR=0x71
I2C_8_MUX_ADDR=0x72
I2C_M2_MUX_ADDR=0x73
I2C_M2CARD_AMB_ADDR=0x4c
M2_CHANNEL0=0x01

M2_MUX_ADDR=$(echo $I2C_M2_MUX_ADDR | cut -c3-4)
M2CARD_AMB_ADDR=$(echo $I2C_M2CARD_AMB_ADDR | cut -c3-4)

for (( i=0; i<7; i++ ))
do
        NUM=$(($MUX_BASE + $i))
        MUX=$(printf "0x%x" $NUM)
 
        i2c_set7=$(i2cset -y 7 $I2C_7_MUX_ADDR $MUX 2>&1)
        i2c_get7=$(i2cget -y 7 $I2C_7_MUX_ADDR 2>&1)
        # if i2c command fail, reset ssd mux by enable GPIOQ6
        if ([[ $i2c_set7 == *Error* || $i2c_get7 == *Error* ]])
        then
                gpio_set Q6 0
                sleep 1
                gpio_set Q6 1
                if ([[ $i == 0 ]])
                then
                        break
                fi
                continue
        fi

        i2c_scan7=$(i2cdetect -y 7)
        if ([[ $i2c_scan7 == *$M2_MUX_ADDR* ]])
        then
                i2cset -y 7 $I2C_M2_MUX_ADDR $M2_CHANNEL0
                i2c_scan7=$(i2cdetect -y 7)
                if ([[ $i2c_scan7 == *$M2CARD_AMB_ADDR* ]])
                then
                        kv_set "ssd_sku_info" "M2"
                        exit 0
                fi
        fi
done

for (( i=0; i<8; i++ ))
do
        NUM=$(($MUX_BASE + $i))
        MUX=$(printf "0x%x" $NUM)

        i2c_set8=$(i2cset -y 8 $I2C_8_MUX_ADDR $MUX 2>&1)
        i2c_get8=$(i2cget -y 8 $I2C_8_MUX_ADDR 2>&1)
        # if i2c command fail, reset ssd mux by enable GPIOQ6
        if ([[ $i2c_set8 == *Error* || $i2c_get8 == *Error* ]])
        then
                gpio_set Q6 0
                sleep 1
                gpio_set Q6 1
                if ([[ $i == 0 ]])
                then
                        break
                fi
                continue
        fi

        i2c_scan8=$(i2cdetect -y 8)
        if ([[ $i2c_scan8 == *$M2_MUX_ADDR* ]])
        then
                i2cset -y 8 $I2C_M2_MUX_ADDR $M2_CHANNEL0
                i2c_scan8=$(i2cdetect -y 8)
                if ([[ $i2c_scan8 == *$M2CARD_AMB_ADDR* ]])
                then
                        kv_set "ssd_sku_info" "M2"
                        exit 0
                fi
        fi
done

kv_set "ssd_sku_info" "U2"

exit 0
