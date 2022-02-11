#!/bin/bash

BUS3_CHAN2_EXTEND_BUSNUM=34
BUS3_CHAN3_EXTEND_BUSNUM=35
U11_ADDR=0x60
U13_ADDR=0x6d
U95_ADDR=0x6f

# Disable U13 IDT_9QXL2001BNHGI8 - DIF0~3
U13_byte1=$(/usr/sbin/i2craw -w 0 -r 10 $BUS3_CHAN2_EXTEND_BUSNUM $U13_ADDR | sed '$!d' | tr -s ' '| cut -d' ' -f4)
U13_byte1_val=${U13_byte1:2:2}
mod_U13_byte1_val=$[$((16#${U13_byte1_val})) & $((16#F0))]
# Disable U13 IDT_9QXL2001BNHGI8 - DIF13~15
U13_byte2=$(/usr/sbin/i2craw -w 0 -r 10 $BUS3_CHAN2_EXTEND_BUSNUM $U13_ADDR | sed '$!d' | tr -s ' '| cut -d' ' -f5)
U13_byte2_val=${U13_byte2:2:2}
mod_U13_byte2_val=$[$((16#${U13_byte2_val})) & $((16#1F))]

/usr/sbin/i2craw -w "1 2 $mod_U13_byte1_val $mod_U13_byte2_val" $BUS3_CHAN2_EXTEND_BUSNUM $U13_ADDR

# Disable U95 IDT_9QXL2001BNHGI8 - DIF16~18
U95_byte0=$(/usr/sbin/i2craw -w 0 -r 10 $BUS3_CHAN2_EXTEND_BUSNUM $U95_ADDR | sed '$!d' | tr -s ' '| cut -d' ' -f3)
U95_byte0_val=${U95_byte0:2:2}
mod_U95_byte0_val=$[$((16#${U95_byte0_val})) & $((16#C7))]

# Disable U95 IDT_9QXL2001BNHGI8 - DIF3~4
U95_byte1=$(/usr/sbin/i2craw -w 0 -r 10 $BUS3_CHAN2_EXTEND_BUSNUM $U95_ADDR | sed '$!d' | tr -s ' '| cut -d' ' -f4)
U95_byte1_val=${U95_byte1:2:2}
mod_U95_byte1_val=$[$((16#${U95_byte1_val})) & $((16#E7))]

# Disable U95 IDT_9QXL2001BNHGI8 - DIF13~15
U95_byte2=$(/usr/sbin/i2craw -w 0 -r 10 $BUS3_CHAN2_EXTEND_BUSNUM $U95_ADDR | sed '$!d' | tr -s ' '| cut -d' ' -f5)
U95_byte2_val=${U95_byte2:2:2}
mod_U95_byte2_val=$[$((16#${U95_byte2_val})) & $((16#1F))]

/usr/sbin/i2craw -w "0 3 $mod_U95_byte0_val $mod_U95_byte1_val $mod_U95_byte2_val" $BUS3_CHAN2_EXTEND_BUSNUM $U95_ADDR

# Disable U11 IDT_9FGP204BKLFT - RMII0, RMII5, RGMII1 and RGMII0
U11_byte1=$(/usr/sbin/i2craw -w 0 -r 10 $BUS3_CHAN3_EXTEND_BUSNUM $U11_ADDR | sed '$!d' | tr -s ' '| cut -d' ' -f4)
U11_byte1_val=${U11_byte1:2:2}
mod_U11_byte1_val=$[$((16#${U11_byte1_val})) & $((16#79))]

/usr/sbin/i2craw -w "1 1 $mod_U11_byte1_val" $BUS3_CHAN3_EXTEND_BUSNUM $U11_ADDR
