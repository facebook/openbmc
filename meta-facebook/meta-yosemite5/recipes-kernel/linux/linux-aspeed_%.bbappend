FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

#
# Use latest i3c hub driver from local folder for now,
# until DTS is updated in other projects already using it.
#
SRC_URI:remove = " \
    file://0202-dt-bindings-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB.patch \
    file://0203-i3c-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB-driver.patch \
"

SRC_URI:append = " \
    file://1000-add-meta-yosemite5-bmc-dts.patch \
    file://1001-ARM-dts-aspeed-yosemite5-Add-lpc_pcc-node.patch \
    file://1002-ARM-dts-aspeed-yosemite5-configure-tach-channels-for-max31790.patch \
    file://1003-ARM-dts-aspeed-yosemite5-configure-dimm-ids-for-sbrmi-node.patch \
    file://1004-ARM-dts-aspeed-yosemite5-set-ncsi-package-equal-to-1.patch \
    file://1005-ARM-dts-aspeed-yosemite5-add-i3c-nodes.patch \
    file://1006-ARM-dts-aspeed-yosemite5-increase-i2c4-i2c12-bus-speed-to-400-khz.patch \
    file://1007-ARM-dts-aspeed-yosemite5-remove-ambiguous-power-monitor-dts-nodes.patch \
    file://1008-ARM-dts-aspeed-yosemite5-add-new-sgpio-line-names-and-rename-signal.patch \
    file://1009-ARM-dts-aspeed-yosemite5-add-ipmb-node-for-ocp-debug-card.patch \
    file://1010-ARM-dts-aspeed-yosemite5-correct-power-monitor-shunt-resistor.patch \
    file://1011-ARM-dts-aspeed-yosemite5-add-x4-e1-s-expansion-board-i2c-mux.patch \
    file://1012-enable-uart-dma.patch \
    file://0202-dt-bindings-i3c-Add-i3c-hub-support.patch \
    file://0203-i3c-Add-driver-for-i3c-hub-device.patch \
    file://yosemite5-local.cfg \
"
