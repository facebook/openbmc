FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://rotate_logfile \
"

MTERM_LOG_FILES := "mTerm_wedge"

do_install_append() {
  dst="${D}/usr/local/fbpackages/rotate"
  install -m 755 ${WORKDIR}/rotate_logfile ${dst}/logfile
}

FILES_${PN} += "/usr/local/fbpackages/rotate"
