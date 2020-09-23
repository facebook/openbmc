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
    file://vboot_common.py \
    file://image_meta.py \
    file://measure.py \
  "

S = "${WORKDIR}"

PR = "r0"

CFLAGS += '${@bb.utils.contains("MACHINE_FEATURES", "tpm2", "-DCONFIG_TPM_V2", "", d)}'
CFLAGS += '${@bb.utils.contains("MACHINE_FEATURES", "tpm1", "-DCONFIG_TPM_V1", "", d)}'

DEPENDS = "python3 libvbs libkv"
RDEPENDS_${PN}-python3 += "python3-core"
RDEPENDS_${PN} += "libvbs libkv"

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
  install -m 644 vboot_common.py ${D}${PYTHON_SITEPACKAGES_DIR}/
  install -m 644 image_meta.py ${D}${PYTHON_SITEPACKAGES_DIR}/
  install -m 0755 measure.py ${D}${PYTHON_SITEPACKAGES_DIR}/

  install -d ${D}/usr/local/bin
  install -m 0755 vboot-util ${D}/usr/local/bin/vboot-util
  install -m 0755 vboot-check ${D}/usr/local/bin/vboot-check

  install -d ${D}/usr/bin
  ln -snf ${PYTHON_SITEPACKAGES_DIR}/measure.py ${D}/usr/bin/mboot-check
  ln -snf /usr/local/bin/vboot-util ${D}/usr/bin/vboot-util
  ln -snf /usr/local/bin/vboot-check ${D}/usr/bin/vboot-check


}

FILES_${PN} += "${PYTHON_SITEPACKAGES_DIR}"
FILES_${PN} += "/usr/local/bin/"
FILES_${PN} += "/usr/bin/"
