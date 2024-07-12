FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://1001-minerva-modify-i2c-2-clock-percentage.patch \
    file://1002-ARM-dts-aspeed-minerva-add-host0-ready-pin.patch \
"
