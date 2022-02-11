FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://rotate_logfile \
            file://rotate_cri_sel \
            file://rotate_console_log \
"

MTERM_LOG_FILES := "mTerm_wedge"

do_install:append() {
  dst="${D}/usr/local/fbpackages/rotate"
  install -m 755 ${WORKDIR}/rotate_logfile ${dst}/logfile
  install -m 755 ${WORKDIR}/rotate_cri_sel ${dst}/cri_sel
  install -m 755 ${WORKDIR}/rotate_console_log ${dst}/console_log
}

FILES:${PN} += "/usr/local/fbpackages/rotate"
