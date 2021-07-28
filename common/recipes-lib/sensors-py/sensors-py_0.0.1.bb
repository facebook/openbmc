SUMMARY = "Python bindings for lm-sensors."
DESCRIPTION = "sensors.py"
LICENSE = "LGPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

DEPENDS = "python3 libobmc-sensors"
FILESEXTRAPATHS_prepend := "${THISDIR}/patches:"

SRC_URI = "https://github.com/paroj/sensors.py/archive/refs/heads/master.zip \
           file://001-load-so-fix.patch \
          "
SRC_URI[sha256sum] = "38d7b3736eb933999647344ea7fa43426dd6f86fbd929c2a0c59af9762e29005"

S = "${WORKDIR}/sensors.py-master"

inherit python3-dir

do_install() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${S}/sensors.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}
FILES_${PN} = "${PYTHON_SITEPACKAGES_DIR}/sensors.py"
