FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://1007-ARM-dts-aspeed-bletchley-Sort-i2c-device-nodes-by-ad.patch \
    file://1008-ARM-dts-aspeed-bletchley-Fix-style-warnings.patch \
    file://1009-ARM-dts-aspeed-bletchley-Add-second-source-PCA9532-L.patch \
    file://1010-ARM-dts-aspeed-bletchley-Add-second-source-ISL1208-R.patch \
    file://1011-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
    file://1012-rtc-pcf85363-Add-error-checking-to-regmap-calls-in-p.patch \
    file://bletchley-local.cfg \
"
