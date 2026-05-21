FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-dt-bindings-arm-aspeed-Add-Meta-Rainiera6-board.patch \
    file://0002-ARM-dts-aspeed-rainiera6-Add-Meta-Rainiera6-BMC.patch \
"