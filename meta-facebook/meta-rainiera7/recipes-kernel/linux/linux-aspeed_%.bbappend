FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://0001-arm64-dts-aspeed-Add-rainiera7-dts.patch \
    file://0002-arm64-dts-aspeed-rainiera7-Enable-i2c-slave-timeout.patch \
    file://0003-arm64-dts-aspeed-rainiera7-configure-tach-channels-f.patch \
    file://0004-i3c-add-aspeed-mipi-i3c-hci-driver.patch \
    file://0005-arm64-dts-add-ast2700-mipi-i3c-hci-controller-nodes.patch \
    file://0006-arm64-dts-aspeed-Add-the-i3c-nodes-to-rainiera7-dts.patch \
    file://defconfig \
    file://rainiera7-local.cfg \
    file://rainiera7.cfg \
"
