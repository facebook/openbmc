FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0500-arm-dts-ast2600-evb-disable-MDIO-function.patch \
	    file://0503-ARM-dts-Revise-the-default-baudrate.patch \
           "

