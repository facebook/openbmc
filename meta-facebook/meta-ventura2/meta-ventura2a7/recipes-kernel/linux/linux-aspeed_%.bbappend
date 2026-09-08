FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " \
    file://0001-meta-facebook-ventura2a7-Add-aspeed-bmc-facebook-ven.patch \
    file://defconfig \
    file://ventura2a7.cfg \
"
