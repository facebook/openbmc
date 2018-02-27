FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://rsyslog.conf \
            file://rotate_logfile \
            file://rotate_cri_sel \
            file://rotate_console_log \
            file://rsyslog.logrotate \
            file://rsyslog-8.18.0/runtime/stream.c \
"

do_install_append() {
  dst="${D}/usr/local/fbpackages/rotate"
  install -d $dst
  install -m 755 ${WORKDIR}/rotate_logfile ${dst}/logfile
  install -m 755 ${WORKDIR}/rotate_cri_sel ${dst}/cri_sel
  install -m 755 ${WORKDIR}/rotate_console_log ${dst}/console_log
  install -m 644 ${WORKDIR}/rsyslog.conf ${D}${sysconfdir}/rsyslog.conf
  install -m 644 ${WORKDIR}/rsyslog.logrotate ${D}${sysconfdir}/logrotate.rsyslog

  sed -i "s/__PLATFORM_NAME__/${MACHINE}/g" ${D}${sysconfdir}/rsyslog.conf
}

FILES_${PN} += "/usr/local/fbpackages/rotate"
