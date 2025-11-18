FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://1000-add-meta-yosemite5-bmc-dts.patch \
    file://1001-ARM-dts-aspeed-yosemite5-Add-lpc_pcc-node.patch \
    file://1002-ARM-dts-aspeed-yosemite5-configure-tach-channels-for-max31790.patch \
    file://1003-ARM-dts-aspeed-yosemite5-configure-dimm-ids-for-sbrmi-node.patch \
    file://1004-ARM-dts-aspeed-yosemite5-set-ncsi-package-equal-to-1.patch \
    file://1005-ARM-dts-aspeed-yosemite5-add-i3c-nodes.patch \
    file://yosemite5-local.cfg \
"
