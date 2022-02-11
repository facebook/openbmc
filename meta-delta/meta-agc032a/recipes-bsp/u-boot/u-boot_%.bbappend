FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://agc032a_defconfig.append \
            file://Makefile \
            file://agc032a.dts \
            file://agc032a_defconfig \
            file://agc032a.h \
           "

do_copyfile () {
  cp -v ${WORKDIR}/Makefile ${S}/arch/arm/dts/
  cp -v ${WORKDIR}/agc032a.dts  ${S}/arch/arm/dts/
  cp -v ${WORKDIR}/agc032a_defconfig ${S}/configs/
  cp -v ${WORKDIR}/agc032a.h ${S}/include/configs/
}

addtask copyfile after do_patch before do_configure
