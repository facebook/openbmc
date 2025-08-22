FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://1000-ARM-dts-aspeed-santabarbara-add-lpc_pcc-node.patch \
    file://1001-ARM-dts-aspeed-santabarbara-add-sensor-support-for-e.patch \
    file://1002-ARM-dts-aspeed-santabarbara-Enable-MCTP-for-frontend.patch \
    file://1003-ARM-dts-aspeed-santabarbara-Adjust-LED-configuration.patch \
    file://1004-ARM-dts-aspeed-santabarbara-add-sgpio-line-name-for-.patch \
    file://santabarbara-local.cfg \
"
