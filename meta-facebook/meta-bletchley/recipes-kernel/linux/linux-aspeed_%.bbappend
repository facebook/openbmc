FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://1000-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
    file://1001-ARM-dts-aspeed-bletchley-set-manual-clock-for-i2c13.patch \
    file://1002-ARM-dts-aspeed-bletchley-remove-WDTRST1-assertion-fr.patch \
"
