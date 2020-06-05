FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fby3_ext.h \
            file://fby3_defconfig.append \
           "

do_copyfile () {
  cp -v ${WORKDIR}/fby3_ext.h ${S}/include/configs/
}
addtask copyfile after do_patch before do_configure
