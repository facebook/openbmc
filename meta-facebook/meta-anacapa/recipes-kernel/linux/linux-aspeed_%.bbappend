FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://anacapa-local.cfg \
    file://1000-ARM-dts-aspeed-anacapa-Add-Meta-Anacapa-BMC.patch \
"
