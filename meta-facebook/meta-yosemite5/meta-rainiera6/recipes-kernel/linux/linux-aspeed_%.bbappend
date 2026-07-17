FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-ARM-dts-aspeed-rainiera6-Change-ocp-debug-card.patch \
    file://1001-ARM-dts-aspeed-rainiera6-Enable-i2c-slave-timeout.patch \
    file://1002-ARM-dts-aspeed-rainiera6-configure-tach-channels-for.patch \
    file://1003-ARM-dts-aspeed-rainiera6-Add-I3C-Nodes.patch \
"
