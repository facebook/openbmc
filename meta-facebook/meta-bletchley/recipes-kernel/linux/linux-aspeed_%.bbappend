FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
        file://0000-01-ARM-dts-aspeed-bletchley-rename-flash1-label.patch \
        file://0000-02-ARM-dts-aspeed-bletchley-enable-wdtrst1.patch \
        file://0001-dt-bindings-Add-bindings-for-aspeed-pwm-tach.patch \
        file://0002-pwm-Add-Aspeed-ast2600-PWM-support.patch \
        file://0003-Porting-tach-driver-for-ast26xx-from-Aspeed-linux.patch \
        file://0004-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
        file://0005-aspeed-i2c-add-clock-duty-cycle-property.patch \
        file://0006-dt-bindings-aspeed-i2c-add-properties-for-setting-i2.patch \
        file://0007-ARM-dts-aspeed-bletchley-set-manual-clock-for-i2c13.patch \
        file://0008-drivers-misc-Add-Aspeed-OTP-control-register.patch \
"

