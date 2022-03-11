FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
            file://0001-dt-bindings-Add-bindings-for-aspeed-pwm-tach.patch \
            file://0002-pwm-Add-Aspeed-ast2600-PWM-support.patch \
            file://0003-Porting-tach-driver-for-ast26xx-from-Aspeed-linux.patch \
            file://0004-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
            file://0005-hwmon-adm1275-Allow-setting-sample-averaging.patch \
            file://0006-dt-bindings-hwmon-Add-sample-averaging-properties-fo.patch \
            file://0007-ARM-dts-aspeed-bletchley-add-sample-averaging-for-AD.patch \
           "
