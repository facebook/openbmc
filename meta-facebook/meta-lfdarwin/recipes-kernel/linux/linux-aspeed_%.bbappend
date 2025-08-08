FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " file://lfdarwin-local.cfg"

SRC_URI += " \
    file://0001-ARM-dts-aspeed-Add-Facebook-Darwin-AST2600-BMC.patch \
"
