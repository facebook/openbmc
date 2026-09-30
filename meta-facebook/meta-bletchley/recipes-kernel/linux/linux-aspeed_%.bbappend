FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://1000-ARM-dts-aspeed-bletchley-Sort-i2c-device-nodes-by-ad.patch \
    file://1001-ARM-dts-aspeed-bletchley-Fix-style-warnings.patch \
    file://1002-ARM-dts-aspeed-bletchley-Add-second-source-PCA9532-L.patch \
    file://1003-ARM-dts-aspeed-bletchley-Add-second-source-ISL1208-R.patch \
    file://1004-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
    file://bletchley-local.cfg \
"
