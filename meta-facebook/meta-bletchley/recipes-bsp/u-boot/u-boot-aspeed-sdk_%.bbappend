FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
            file://0001-u-boot-ast2600-fix-for-bletchley.patch \
            file://0001-arm-dts-Aspeed-add-Bletchley-dts.patch \
           "
