FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += " \
    file://tpm_integrationtest \
"
do_install:append() {
  install -m 744 ../tpm_integrationtest ${D}${bindir}/tpm_integrationtest
}

do_configure:prepend() {
    # man8 has a dependency on pod2man.  We don't use manpages, so just
    # erase it.
    echo > ${S}/man/Makefile.am
}
