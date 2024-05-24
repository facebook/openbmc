FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0500-arm-dts-ast2600-evb-disable-MDIO-function.patch \
	    file://0501-mem_test-Support-to-save-mtest-result-in-environm.patch \
	    file://0502-configs-openbmc-Revise-size-of-environment-and-us.patch \
	    file://0503-ARM-dts-Revise-the-default-baudrate.patch \
           "

