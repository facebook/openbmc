FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://1000-bindings-ipmi-ssif-bmc-Add-property-to-adjust-respon.patch \
    file://1001-ipmi-ssif_bmc-Add-support-for-adjustable-response-ti.patch \
    file://1002-i3c-add-aspeed-mipi-i3c-hci-driver.patch \
    file://1003-arm64-dts-add-ast2700-mipi-i3c-hci-controller-nodes-.patch \
    file://1004-arm64-dts-aspeed-Add-rainiera7-dts.patch \
    file://defconfig \
    file://rainiera7-local.cfg \
    file://rainiera7.cfg \
"
