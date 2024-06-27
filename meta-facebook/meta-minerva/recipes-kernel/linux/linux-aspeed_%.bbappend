FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://1001-ARM-dts-aspeed-minerva-change-the-address-of-tmp75.patch \ 
    file://1002-ARM-dts-aspeed-minerva-change-aliases-for-uart.patch \ 
    file://1003-ARM-dts-aspeed-minerva-add-eeprom-on-i2c-bus.patch \ 
    file://1004-ARM-dts-aspeed-minerva-change-RTC-reference.patch \ 
    file://1005-ARM-dts-aspeed-minerva-enable-mdio3.patch \ 
    file://1006-ARM-dts-aspeed-minerva-remove-unused-bus-and-device.patch \ 
    file://1007-ARM-dts-aspeed-minerva-Define-the-LEDs-node-name.patch \ 
    file://1008-ARM-dts-aspeed-minerva-Add-adc-sensors-for-fan-board.patch \ 
    file://1009-ARM-dts-aspeed-minerva-add-linename-of-two-pins.patch \ 
    file://1010-ARM-dts-aspeed-minerva-enable-ehci0-for-USB.patch \ 
    file://1011-ARM-dts-aspeed-minerva-add-tmp75-sensor.patch \ 
    file://1012-ARM-dts-aspeed-minerva-add-power-monitor-xdp710.patch \ 
    file://1013-ARM-dts-aspeed-minerva-revise-sgpio-line-name.patch \ 
    file://1014-ARM-dts-aspeed-minerva-Switch-the-i2c-bus-number.patch \ 
    file://1015-ARM-dts-aspeed-minerva-remove-unused-power-device.patch \ 
    file://1016-ARM-dts-aspeed-minerva-add-ltc4287-device.patch \ 
    file://1017-ARM-dts-aspeed-minerva-Add-spi-gpio.patch \ 
    file://2001-minerva-modify-i2c-2-clock-percentage.patch \ 
"
