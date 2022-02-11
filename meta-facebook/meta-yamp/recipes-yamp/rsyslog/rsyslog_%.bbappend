FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://rotate_logfile \
"

MTERM_LOG_FILES := "mTerm_wedge"

do_install:append() {
  dst="${D}/usr/local/fbpackages/rotate"
  install -m 755 ${WORKDIR}/rotate_logfile ${dst}/logfile
}

FILES:${PN} += "/usr/local/fbpackages/rotate"
