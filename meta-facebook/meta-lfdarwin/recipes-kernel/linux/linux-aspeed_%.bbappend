FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " file://lfdarwin-local.cfg"

SRC_URI += " \
    file://0001-Add-mac3-entry-and-LFOpenBMC-flash-layout.patch \
"
