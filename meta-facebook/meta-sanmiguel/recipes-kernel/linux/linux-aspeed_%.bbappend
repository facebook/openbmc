FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://sanmiguel-local.cfg \
    file://1001-ipmi-ssif_bmc-add-GPIO-based-alert-mechanism.patch \
    file://1002-bindings-ipmi-ssif-bmc-Add-property-to-adjust-respon.patch \
    file://1003-ipmi-ssif_bmc-Add-support-for-adjustable-response-ti.patch \
    file://1004-dt-bindings-i2c-Add-CP2112-HID-USB-to-SMBus-Bridge.patch \
    file://1005-HID-cp2112-Fwnode-Support.patch \
    file://1006-HID-cp2112-Configure-I2C-Bus-Speed-from-Firmware.patch \
    file://1007-Add-RTL8211F-RMII-to-SGMII-bridge-support.patch \
    file://1008-i2c-muxes-introduce-virtual-mux-host.patch \
"



SRC_URI:append = " \
    file://2001-ARM-dts-aspeed-sanmiguel-Add-SSIF-DTS-properties.patch \
    file://2002-ARM-dts-aspeed-sanmiguel-Add-CP2112-and-downstream-I.patch \
    file://2003-ARM-dts-aspeed-sanmiguel-Add-virtual-mux-host-node.patch \
    "
