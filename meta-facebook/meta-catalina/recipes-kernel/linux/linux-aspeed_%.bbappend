FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://1000-i2c-aspeed-Acknowledge-Tx-ack-late-when-in-SLAVE_REA.patch \
    file://1001-ARM-dts-aspeed-catalina-add-pdb-cpld-io-expander.patch \
    file://1002-ARM-dts-aspeed-catalina-add-extra-ncsi-properties.patch \
    file://1003-ARM-dts-aspeed-catalina-add-ipmb-dev-node.patch \
    file://1004-ARM-dts-aspeed-catalina-add-max31790-nodes.patch \
    file://1005-ARM-dts-aspeed-catalina-add-raa228004-nodes.patch \
"
