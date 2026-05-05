FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://anacapa-local.cfg \
    file://1000-ARM-dts-aspeed-anacapa-Add-Meta-Anacapa-BMC.patch \
    file://1001-ARM-dts-aspeed-anacapa-config-ncsi-device.patch \
    file://1002-ARM-dts-aspeed-anacapa-Add-eeprom-device-node.patch \
    file://1003-ARM-dts-aspeed-anacapa-enable-APML-I3C-bus.patch \
    file://1004-Enable-NIC-MCTP-over-I2C.patch \
    file://1005-dt-bindings-arm-aspeed-add-Anacapa-EVT1-EVT2-board.patch \
    file://1006-ARM-dts-aspeed-anacapa-add-EVT1-devicetree-and-point.patch \
    file://1007-ARM-dts-aspeed-anacapa-add-EVT2-devicetree-and-updat.patch \
    file://1008-ARM-dts-aspeed-anacapa-evt2-add-lpc_pcc-device.patch \
    file://1009-ARM-dts-aspeed-anacapa-evt2-config-ncsi-device.patch \
    file://1010-ARM-dts-aspeed-anacapa-evt2-enable-APML-I3C-bus.patch \
    file://1011-Enable-NIC-MCTP-over-I2C.patch \
"