FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-ARM-dts-aspeed-rainiera6-Enable-i2c-slave-timeout.patch \
    file://0002-ARM-dts-aspeed-rainiera6-configure-tach-channels-for.patch \
    file://0003-ARM-dts-aspeed-rainiera6-Add-I3C-Nodes.patch \
"