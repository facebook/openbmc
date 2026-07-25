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
    file://1009-Squash-of-the-work-in-progress-series-on-this-branch.patch \
    file://1010-virtual-mux-fail-fast-when-downstream-device-is-abse.patch \
    file://1011-Replace-cp2112-sync-xfer-with-async-workqueue.patch \
    file://1012-Rate-limit-cp2112-GPIO-poll-error-log.patch \
    file://1013-hid-cp2112-validate-data-length-byte.patch \
"



SRC_URI:append = " \
    file://2001-ARM-dts-aspeed-sanmiguel-Add-SSIF-DTS-properties.patch \
    file://2002-ARM-dts-aspeed-sanmiguel-Add-CP2112-and-downstream-I.patch \
    file://2003-ARM-dts-aspeed-sanmiguel-Add-virtual-mux-host-node.patch \
    file://2004-ARM-dts-aspeed-sanmiguel-fix-PDB-HSC-shunt-resistor.patch \
    file://2005-ARM-dts-aspeed-sanmiguel-add-current-range-property-.patch \
    file://2006-ARM-dts-aspeed-sanmiguel-fix-PWRGD_RMC_N-active-low-.patch \
    file://2007-ARM-dts-aspeed-sanmiguel-Fix-FIO-IOEXP-interrupt-pin.patch \
    "
