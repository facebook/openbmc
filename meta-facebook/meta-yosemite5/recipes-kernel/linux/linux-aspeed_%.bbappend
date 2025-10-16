FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://1000-add-meta-yosemite5-bmc-dts.patch \
    file://yosemite5-local.cfg \
"
