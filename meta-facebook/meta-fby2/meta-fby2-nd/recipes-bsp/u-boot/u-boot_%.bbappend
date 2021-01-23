FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fby2_ext.h \
            file://fby2_defconfig.append \
           "

do_copyfile () {
  cp -v ${WORKDIR}/fby2_ext.h ${S}/include/configs/
}
addtask copyfile after do_patch before do_configure
