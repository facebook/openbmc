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
    file://Makefile \
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

LDFLAGS += " -lfdt"
DEPENDS = "python3 libvbs libkv dtc"
RDEPENDS:${PN}-python3 += "python3-core"
RDEPENDS:${PN} += "libvbs libkv"

PACKAGES += "${PN}-python3"
inherit distutils3 python3-dir

distutils3_do_configure(){
    :
}

do_compile() {
  oe_runmake -C ${S}
}

do_install() {
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

  install -d ${D}/usr/local/bin
  install -m 0755 ${S}/vboot-util ${D}/usr/local/bin/vboot-util
  install -m 0755 ${S}/vboot-check ${D}/usr/local/bin/vboot-check

  install -d ${D}/usr/bin
  ln -snf ${PYTHON_SITEPACKAGES_DIR}/measure.py ${D}/usr/bin/mboot-check
  ln -snf ${PYTHON_SITEPACKAGES_DIR}/memdump.py ${D}/usr/bin/phymemdump
  ln -snf /usr/local/bin/vboot-util ${D}/usr/bin/vboot-util
  ln -snf /usr/local/bin/vboot-check ${D}/usr/bin/vboot-check


}

FILES:${PN} += "${PYTHON_SITEPACKAGES_DIR}"
FILES:${PN} += "/usr/local/bin/"
FILES:${PN} += "/usr/bin/"
