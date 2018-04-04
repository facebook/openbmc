FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fby2_ext.h \
           "

do_copyfile () {
  cp -v ${WORKDIR}/fby2_ext.h ${S}/include/configs/
}
addtask copyfile after do_patch before do_configure

