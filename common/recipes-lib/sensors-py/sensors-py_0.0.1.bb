SUMMARY = "Python bindings for lm-sensors."
DESCRIPTION = "sensors.py"
LICENSE = "LGPLv2"

# The license LGPL-2.1 was removed in Hardknott.
# Use LGPL-2.1-only instead.
def lic_file_name(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

    return "LGPL-2.1-only;md5=1a6d268fd218675ffea8be556788b780"

LIC_FILES_CHKSUM = "\
    file://${COREBASE}/meta/files/common-licenses/${@lic_file_name(d)} \
    "

DEPENDS = "python3 libobmc-sensors"
FILESEXTRAPATHS:prepend := "${THISDIR}/patches:"

SRC_URI = "git://github.com/paroj/sensors.py;protocol=https;branch=master \
           file://001-load-so-fix.patch \
          "
SRCREV = "4001b2c1ee00d9d7753827609f98920461a364b7"
S = "${WORKDIR}/git"

inherit python3-dir

do_install() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${S}/sensors.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}
FILES:${PN} = "${PYTHON_SITEPACKAGES_DIR}/sensors.py"
