LINUX_VERSION_EXTENSION = "-agc032a"

COMPATIBLE_MACHINE = "agc032a"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://agc032a.cfg \
            file://aspeed-bmc-delta-agc032a.dts \
            file://Makefile \
           "

do_copyfile () {
  cp -v ${WORKDIR}/Makefile ${S}/arch/arm/boot/dts
  cp -v ${WORKDIR}/aspeed-bmc-delta-agc032a.dts ${S}/arch/arm/boot/dts/
}

addtask copyfile after do_patch before do_configure

