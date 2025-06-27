FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://1000-i2c-aspeed-Acknowledge-Tx-ack-late-when-in-SLAVE_REA.patch \
    file://1001-ipmi-ssif_bmc-add-GPIO-based-alert-mechanism.patch \
    file://1002-bindings-ipmi-ssif-bmc-Add-property-to-adjust-respon.patch \
    file://1003-ipmi-ssif_bmc-Add-support-for-adjustable-response-ti.patch \
    file://1004-ARM-dts-aspeed-catalina-add-extra-ncsi-properties.patch \
    file://1005-ARM-dts-aspeed-catalina-add-ipmb-dev-node.patch \
    file://1006-ARM-dts-aspeed-catalina-add-max31790-nodes.patch \
    file://1007-ARM-dts-aspeed-catalina-add-raa228004-nodes.patch \
    file://1008-ARM-dts-aspeed-catalina-enable-ssif-alert-pin.patch \
    file://1009-ARM-dts-aspeed-catalina-enable-uart-dma-mode.patch \
    file://1010-ARM-dts-aspeed-catalina-update-pdb-board-cpld-ioexp-.patch \
    file://1011-ARM-dts-aspeed-catalina-add-hdd-board-cpld-ioexp.patch \
    file://1012-ARM-dts-aspeed-catalina-Add-Delta-brick-nodes.patch \
    file://1013-ARM-dts-aspeed-catalina-Increase-SSIF-response-timeo.patch \
    file://1014-i2c-ast2600-support-ssif.patch \
    file://1015-ARM-dts-aspeed-catalina-enable-i2c11-byte-mode.patch \
    file://catalina-local.cfg \
"
