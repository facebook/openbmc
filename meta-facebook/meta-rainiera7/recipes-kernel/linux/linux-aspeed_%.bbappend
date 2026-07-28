FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://0001-arm64-dts-aspeed-Add-rainiera7-dts.patch \
    file://0002-arm64-dts-aspeed-rainiera7-Enable-i2c-slave-timeout.patch \
    file://0003-arm64-dts-aspeed-rainiera7-configure-tach-channels-f.patch \
    file://defconfig \
    file://rainiera7-local.cfg \
    file://rainiera7.cfg \
"
