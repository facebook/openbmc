FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://rsyslog.conf \
            file://rotate_logfile \
            file://rotate_cri_sel \
"

do_install_append() {
  dst="${D}/usr/local/fbpackages/rotate"
  install -d $dst
  install -m 755 ${WORKDIR}/rotate_logfile ${dst}/logfile
  install -m 755 ${WORKDIR}/rotate_cri_sel ${dst}/cri_sel
}

FILES_${PN} += "/usr/local/fbpackages/rotate"
