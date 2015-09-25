FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://patch-2013.07/0000-u-boot-aspeed-064.patch \
            file://patch-2013.07/0001-u-boot-openbmc.patch \
            file://patch-2013.07/0002-Create-snapshot-of-OpenBMC.patch;striplevel=6 \
           "
