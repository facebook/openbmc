FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-ARM-dts-aspeed-rainiera6-Config-ocp-debug-card.patch \
    file://0002-ARM-dts-aspeed-rainiera6-Add-EMC1403-temperature-sen.patch \
    file://1000-bindings-ipmi-ssif-bmc-Add-property-to-adjust-respon.patch \
    file://1001-ipmi-ssif_bmc-Add-support-for-adjustable-response-ti.patch \
    file://1002-ARM-dts-aspeed-rainiera6-Enable-i2c-slave-timeout.patch \
    file://1003-ARM-dts-aspeed-rainiera6-configure-tach-channels-for.patch \
    file://1004-ARM-dts-aspeed-rainiera6-Add-I3C-Nodes.patch \
    file://1005-ARM-dts-aspeed-rainiera6-enable-i2c1-byte-mode-for-S.patch \
    file://1006-ARM-dts-aspeed-rainiera6-increase-ssif-response-time.patch \
"
