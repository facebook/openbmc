SUMMARY = "The tpm-tools package contains commands to allow the platform administrator the ability to manage and diagnose the platform's TPM."
DESCRIPTION = " \
  The tpm-tools package contains commands to allow the platform administrator \
  the ability to manage and diagnose the platform's TPM.  Additionally, the \
  package contains commands to utilize some of the capabilities available \
  in the TPM PKCS#11 interface implemented in the openCryptoki project. \
  "
SECTION = "tpm"
LICENSE = "CPL-1.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=059e8cd6165cb4c31e351f2b69388fd9"
DEPENDS = "libtspi openssl"

SRC_URI += " \
    http://downloads.sourceforge.net/project/trousers/${BPN}/${PV}/${BP}.tar.gz \
    file://tpm-tools-extendpcr.patch \
    file://03-fix-bool-error-parseStringWithValues.patch \
    file://0001-Fix-formating-issues.patch \
    file://tpm_integrationtest \
"

SRC_URI[md5sum] = "85a978c4e03fefd4b73cbeadde7c4d0b"
SRC_URI[sha256sum] = "66eb4ff095542403db6b4bd4b574e8a5c08084fe4e9e5aa9a829ee84e20bea83"

do_install_append() {
  install -m 755 src/tpm_mgmt/tpm_startup ${D}${sbindir}/tpm_startup
  install -m 744 src/tpm_mgmt/tpm_reset ${D}${sbindir}/tpm_reset
  install -m 744 ../tpm_integrationtest ${D}${bindir}/tpm_integrationtest
}

inherit autotools gettext
