FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0500-arm-dts-ast2600-evb-disable-MDIO-function.patch \
            file://0502-arm-dts-ast2600-evb-Enable-alternate-boot.patch \
            file://0503-arm-dts-ast2600-evb-enable-ecc-function.patch \
           "

