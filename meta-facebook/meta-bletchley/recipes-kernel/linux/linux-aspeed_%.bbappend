FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://0501-ARM-dts-aspeed-bletchley-set-manual-clock-for-i2c13.patch \
"

# Need to rebase this with the latest PWM driver.
#    file://0500-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
#
