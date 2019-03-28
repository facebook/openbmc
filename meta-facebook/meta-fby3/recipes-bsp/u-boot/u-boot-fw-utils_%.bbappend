FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fby3_ext.h \
           "

do_copyfile () {
  cp -v ${WORKDIR}/fby3_ext.h ${S}/include/configs/
}
addtask copyfile after do_patch before do_configure
