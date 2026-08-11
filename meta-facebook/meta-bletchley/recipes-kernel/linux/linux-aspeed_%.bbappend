FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://1000-ARM-dts-aspeed-bletchley-remove-WDTRST1-assertion-fr.patch \
    file://1001-ARM-dts-aspeed-bletchley-Use-generic-node-names.patch \
    file://1002-ARM-dts-aspeed-bletchley-Fix-SPI-GPIO-property-names.patch \
    file://1003-ARM-dts-aspeed-bletchley-Remove-unused-pca9539-prope.patch \
    file://1004-ARM-dts-aspeed-bletchley-Remove-unused-i2c13-propert.patch \
    file://1005-ARM-dts-aspeed-bletchley-Fix-ADC-vref-property-names.patch \
    file://1006-ARM-dts-aspeed-bletchley-Remove-try-power-role-from-.patch \
    file://1007-ARM-dts-aspeed-bletchley-Sort-i2c-device-nodes-by-ad.patch \
    file://1008-ARM-dts-aspeed-bletchley-Fix-style-warnings.patch \
    file://1009-ARM-dts-aspeed-bletchley-Add-second-source-PCA9532-L.patch \
    file://1010-ARM-dts-aspeed-bletchley-Add-second-source-ISL1208-R.patch \
    file://1011-ARM-dts-aspeed-bletchley-enable-PWM-and-TACH-support.patch \
    file://1012-rtc-pcf85363-Add-error-checking-to-regmap-calls-in-p.patch \
    file://bletchley-local.cfg \
"
