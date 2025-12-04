FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://1000-add-meta-yosemite5-bmc-dts.patch \
    file://1001-ARM-dts-aspeed-yosemite5-Add-lpc_pcc-node.patch \
    file://1002-ARM-dts-aspeed-yosemite5-configure-tach-channels-for-max31790.patch \
    file://1003-ARM-dts-aspeed-yosemite5-configure-dimm-ids-for-sbrmi-node.patch \
    file://1004-ARM-dts-aspeed-yosemite5-set-ncsi-package-equal-to-1.patch \
    file://1005-ARM-dts-aspeed-yosemite5-add-i3c-nodes.patch \
    file://1006-ARM-dts-aspeed-yosemite5-increase-i2c4-i2c12-bus-speed-to-400-khz.patch \
    file://1007-ARM-dts-aspeed-yosemite5-update-sensor-configuration.patch \
    file://1008-ARM-dts-aspeed-yosemite5-rename-sgpio-p0_i3c_apml_alert_l.patch \
    file://1009-ARM-dts-aspeed-yosemite5-add-ipmb-node-for-ocp-debug-card.patch \
    file://1010-enable-uart-dma.patch \
    file://yosemite5-local.cfg \
"
