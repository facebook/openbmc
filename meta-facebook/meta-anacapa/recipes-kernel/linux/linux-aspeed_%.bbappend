FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://anacapa-local.cfg \
    file://1000-ARM-dts-aspeed-anacapa-Add-Meta-Anacapa-BMC.patch \
    file://1001-ARM-dts-aspeed-anacapa-config-ncsi-device.patch \
    file://1002-ARM-dts-aspeed-anacapa-Add-eeprom-device-node.patch \
    file://1003-ARM-dts-aspeed-anacapa-enable-APML-I3C-bus.patch \
"
