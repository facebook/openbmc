FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configuration-ventura2-Add-MCTPI2C-for-E810.patch \
"
