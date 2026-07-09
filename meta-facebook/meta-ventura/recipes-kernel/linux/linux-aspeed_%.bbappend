FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " file://ventura-local.cfg"

SRC_URI += " \
    file://0001-meta-facebook-ventura-Add-Linux-device-tree-related-.patch \
    file://0002-ARM-dts-aspeed-ventura-modify-dts-for-PVT-stage.patch \
    file://0003-ARM-dts-aspeed-ventura-add-ipmb-dev-node.patch \
    file://0004-ARM-dts-aspeed-ventura-modify-dts-for-Pilot-stage.patch \
    file://0005-ARM-dts-aspeed-ventura-modify-dts-for-PVT-exit-stage.patch \
    file://0007-ARM-dts-aspeed-ventura-add-missing-cable-presence-gpios-v2.patch \
    file://0008-ARM-dts-aspeed-Ventura-Enable-i2c-slave-timeout.patch \
    file://0009-ARM-dts-aspeed-ventura-add-the-0x11-ioexp-to-i2c10.patch \
"
