FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

def ipmitool_patch_dir(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in ['rocko', 'dunfell', 'kirkstone']:
        return "1.8.18"
    return "1.8.19"

DEPENDS:append:openbmc-fb = " libipmb libipmi libpal"
RDEPENDS:append:${PN}:openbmc-fb = " libipmb libipmi libpal"
LDFLAGS:append:openbmc-fb = " -lipmb -lipmi -lpal"

SRC_URI:append:openbmc-fb = " \
    file://${@ipmitool_patch_dir(d)}/0001-add-facebook-openbmc-interface.patch \
    file://obmc \
    "

do_copyfile() {
}

do_copyfile:append:openbmc-fb () {
  cp -vr ${WORKDIR}/obmc ${S}/src/plugins/
}
addtask copyfile after do_patch before do_configure
