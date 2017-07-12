FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

DEPENDS_append = " libipmb libipmi libpal"
RDEPENDS_${PN} += " libipmb libipmi libpal"
LDFLAGS_append = " -lipmb -lipmi -lpal"

SRC_URI += "file://00-add_obmc_intf.patch \
            file://obmc \
           "

do_copyfile () {
  cp -vr ${WORKDIR}/obmc ${WORKDIR}/ipmitool-*/src/plugins/
}
addtask copyfile after do_patch before do_configure
