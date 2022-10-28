SUMMARY = "Verified boot utilities"
DESCRIPTION = "This includes the verified-boot status and update utilities"
SECTION = "base"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://COPYING;md5=c110423312df5eaf34c8925fc0995bd4"

LOCAL_URI += " \
    file://COPYING \
    file://pyfdt/__init__.py \
    file://pyfdt/pkcs11.py \
    file://pyfdt/pyfdt.py \
    file://meson.build \
    file://vboot-util.c \
    file://vboot-check \
    file://vboot_common.py \
    file://image_meta.py \
    file://measure.py \
    file://measure_func.py \
    file://memdump.py \
    file://tpm_event_log.py \
    "

PR = "r0"

CFLAGS:append:mf-tpm1 = " -DCONFIG_TPM_V1"
CFLAGS:append:mf-tpm2 = " -DCONFIG_TPM_V2"

DEPENDS = "python3 libvbs libkv dtc"
RDEPENDS:${PN}-python3 += "python3-core"

PACKAGES += "${PN}-python3"
inherit setuptools3 python3-dir

distutils3_do_configure(){
    :
}
setuptools3_do_configure(){
    :
}

inherit meson pkgconfig
inherit legacy-packages

do_install:append() {
  install -d ${D}${PYTHON_SITEPACKAGES_DIR}/pyfdt

  for file in ${S}/pyfdt/*.py; do
    install -m 644 "$file" ${D}${PYTHON_SITEPACKAGES_DIR}/pyfdt/
  done
  install -m 644 ${S}/vboot_common.py ${D}${PYTHON_SITEPACKAGES_DIR}/
  install -m 644 ${S}/image_meta.py ${D}${PYTHON_SITEPACKAGES_DIR}/
  install -m 644 ${S}/measure_func.py ${D}${PYTHON_SITEPACKAGES_DIR}/
  install -m 644 ${S}/tpm_event_log.py ${D}${PYTHON_SITEPACKAGES_DIR}/
  install -m 0755 ${S}/measure.py ${D}${PYTHON_SITEPACKAGES_DIR}/
  install -m 0755 ${S}/memdump.py ${D}${PYTHON_SITEPACKAGES_DIR}/

  install -d ${D}/usr/bin
  ln -snf ${PYTHON_SITEPACKAGES_DIR}/measure.py ${D}/usr/bin/mboot-check
  ln -snf ${PYTHON_SITEPACKAGES_DIR}/memdump.py ${D}/usr/bin/phymemdump
  install -m 0755 ${S}/vboot-check ${D}/usr/bin/vboot-check

  install -d ${D}/usr/local/bin
  ln -snf /usr/bin/vboot-check ${D}/usr/local/bin/vboot-check
}
