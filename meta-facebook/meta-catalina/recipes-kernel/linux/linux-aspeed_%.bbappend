FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://0501-i2c-aspeed-Acknowledge-Tx-ack-late-when-in-SLAVE_REA.patch \
    file://0502-ARM-dts-aspeed-catalina-add-pdb-cpld-io-expander.patch \
    file://0503-ARM-dts-aspeed-catalina-add-extra-ncsi-properties.patch \
    file://0504-ARM-dts-aspeed-catalina-add-ipmb-dev-node.patch \
    file://0505-ARM-dts-aspeed-catalina-add-max31790-nodes.patch \
    file://0506-ARM-dts-aspeed-catalina-add-raa228004-nodes.patch \
"
