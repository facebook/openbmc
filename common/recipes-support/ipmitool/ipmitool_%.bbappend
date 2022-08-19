FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

DEPENDS:append:openbmc-fb = " libipmb libipmi libpal"
RDEPENDS:append:${PN}:openbmc-fb = " libipmb libipmi libpal"
LDFLAGS:append:openbmc-fb = " -lipmb -lipmi -lpal"

SRC_URI:append:openbmc-fb = " file://00-add_obmc_intf.patch \
                              file://obmc \
                            "

do_copyfile() {
}

do_copyfile:append:openbmc-fb () {
  cp -vr ${WORKDIR}/obmc ${WORKDIR}/ipmitool-*/src/plugins/
}
addtask copyfile after do_patch before do_configure
