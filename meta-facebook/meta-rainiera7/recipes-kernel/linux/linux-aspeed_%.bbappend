FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://1000-Arm64-aspeed-dts-add-yosemite5-dts.patch \
    file://defconfig \
    file://rainiera7-local.cfg \
    file://rainiera7.cfg \
"
