FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI += " \
    file://1000-meta-facebook-harma-add-harm-dts.patch \
    file://1001-ARM-dts-aspeed-Harma-Revise-i2c0-duty-cycle.patch \
"

