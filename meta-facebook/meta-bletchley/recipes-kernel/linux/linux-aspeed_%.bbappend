FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
            file://0001-dt-bindings-Add-bindings-for-aspeed-pwm-tach.patch \
            file://0002-pwm-Add-Aspeed-ast2600-PWM-support.patch \
            file://0003-Porting-tach-driver-for-ast26xx-from-Aspeed-linux.patch \
            file://0004-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
            file://0005-ARM-dts-aspeed-bletchley-enable-ehci0-device-node.patch \
            file://0006-ARM-dts-aspeed-bletchley-switch-spi2-driver-to-aspee.patch \
            file://0007-ARM-dts-aspeed-bletchley-Enable-mdio0-bus.patch \
            file://0008-ARM-dts-aspeed-bletchley-update-gpio0-line-names.patch \
            file://0009-ARM-dts-aspeed-bletchley-add-pca9536-node-on-each-sl.patch \
            file://0010-ARM-dts-aspeed-bletchley-add-eeprom-node-on-each-sle.patch \
            file://0011-ARM-dts-aspeed-bletchley-update-fusb302-nodes.patch \
           "

