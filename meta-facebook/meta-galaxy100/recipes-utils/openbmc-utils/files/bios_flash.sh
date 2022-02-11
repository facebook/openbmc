#!/bin/sh
i2cset -f -y 0 0x3e 0x16 0x01
if [ ! -f /dev/spidev5 ]
then
	mknod /dev/spidev5.0 c 153 0
	modprobe spidev
fi
flashrom -p linux_spi:dev=/dev/spidev5.0 -c W25Q64.V
#flashrom -p linux_spi:dev=/dev/spidev5.0 -r bios
