SUMMARY = "Verified boot utilities"
DESCRIPTION = "This includes the verified-boot status and update utilities"
SECTION = "base"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://COPYING;md5=c110423312df5eaf34c8925fc0995bd4"

SRC_URI += " \
    file://COPYING \
    file://pyfdt/__init__.py \
    file://pyfdt/pkcs11.py \
    file://pyfdt/pyfdt.py \
    file://Makefile \
    file://vboot-util.c \
    file://vboot-check \
    "

S = "${WORKDIR}"

DEPENDS = "python3 libvbs"
RDEPENDS_${PN}-python3 += "python3-core"
RDEPENDS_${PN} += "libvbs"

PACKAGES += "${PN}-python3"
inherit distutils3 python3-dir

distutils3_do_configure(){
    :
}

do_compile() {
  # No-op
  make
}

do_install() {
  install -d ${D}${PYTHON_SITEPACKAGES_DIR}/pyfdt

  for file in ${S}/pyfdt/*.py; do
    install -m 644 "$file" ${D}${PYTHON_SITEPACKAGES_DIR}/pyfdt/
  done

  install -d ${D}/usr/local/bin
  install -m 0755 vboot-util ${D}/usr/local/bin/vboot-util
  install -m 0755 vboot-check ${D}/usr/local/bin/vboot-check
}

FILES_${PN} += "${PYTHON_SITEPACKAGES_DIR}"
FILES_${PN} += "/usr/local/bin/vboot-util"
FILES_${PN} += "/usr/local/bin/vboot-check"
