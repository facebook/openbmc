FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " file://ventura2-local.cfg"

SRC_URI += " \
    file://0001-meta-facebook-ventura2-Add-aspeed-bmc-facebook-ventura2.dt.patch \
"
