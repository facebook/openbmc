FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://1000-ARM-dts-aspeed-santabarbara-add-lpc_pcc-node.patch \
    file://santabarbara-local.cfg \
"
