SUMMARY = "Script to allow meson data to be used on targets"
LICENSE = "GPLv2"

# The license GPL-2.0 was removed in Hardknott.
# Use GPL-2.0-only instead.
def lic_file_name(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'zeus', 'dunfell' ]:
        return "GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

    return "GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

LIC_FILES_CHKSUM = "\
    file://${COREBASE}/meta/files/common-licenses/${@lic_file_name(d)} \
    "

SECTION = "libs"

inherit python3native
inherit native

SRC_URI = "file://ptest-meson-crosstarget"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/ptest-meson-crosstarget ${D}${bindir}
}
