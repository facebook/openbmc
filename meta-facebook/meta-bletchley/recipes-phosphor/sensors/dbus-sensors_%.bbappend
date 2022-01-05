FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0001-Add-reporting-of-Relative-Humidity-for-Hwmon.patch \
    file://0002-HwmonTempSensor-add-HDC1080.patch \
    file://0003-Fansensor-Support-ast26xx-pwm-tach.patch \
    "
