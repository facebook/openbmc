FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI += " \
    file://0001-meta-facebook-ventura-Add-Linux-device-tree-related-.patch \
    file://0002-ARM-dts-aspeed-ventura-modify-dts-for-PVT-stage.patch \
    file://0003-ARM-dts-aspeed-ventura-add-ipmb-dev-node.patch \
    file://0004-ARM-dts-aspeed-ventura-modify-dts-for-Pilot-stage.patch \
    file://0005-net-ftgmac100-reset-MAC-by-SCU-on-RMII.patch \
    file://0006-ARM-dts-aspeed-ventura-modify-dts-for-PVT-exit-stage.patch \
"
