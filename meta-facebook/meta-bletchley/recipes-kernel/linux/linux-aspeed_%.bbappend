FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://0500-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
    file://0501-ARM-dts-aspeed-bletchley-set-manual-clock-for-i2c13.patch \
"
