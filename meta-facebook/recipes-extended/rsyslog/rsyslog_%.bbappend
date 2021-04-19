FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://rsyslog.conf \
            file://rotate_emmc \
            file://rotate_logfile \
            file://rotate_cri_sel \
            file://rotate_console_log \
            file://rsyslog.logrotate \
            file://rsyslog-emmc.conf \
            file://rsyslog-mterm.conf \
            file://rsyslog-remote.conf \
"

MTERM_LOG_FILES ?= "mTerm_${MACHINE}"

do_install_append() {
  dst="${D}/usr/local/fbpackages/rotate"
  rsysconf="${D}${sysconfdir}/rsyslog.d"
  install -d $dst
  install -d ${rsysconf}
  install -m 755 ${WORKDIR}/rotate_logfile ${dst}/logfile
  install -m 755 ${WORKDIR}/rotate_cri_sel ${dst}/cri_sel
  install -m 755 ${WORKDIR}/rotate_console_log ${dst}/console_log

  install -m 644 ${WORKDIR}/rsyslog.conf ${D}${sysconfdir}/rsyslog.conf
  sed -i "s/__OPENBMC_VERSION__/${OPENBMC_VERSION}/g" ${D}${sysconfdir}/rsyslog.conf

  install -m 644 ${WORKDIR}/rsyslog-remote.conf ${rsysconf}/remote.conf

  for mterm_logfile in ${MTERM_LOG_FILES}; do
    conf=${rsysconf}/$mterm_logfile.conf
    install -m 644 ${WORKDIR}/rsyslog-mterm.conf ${conf}
    sed -i "s/__MTERM_LOG_FILE__/${mterm_logfile}/g;" ${conf}
  done

  if ! echo ${MACHINE_FEATURES} | awk "/emmc/ {exit 1}"; then
    install -m 644 ${WORKDIR}/rsyslog-emmc.conf ${rsysconf}/emmc.conf
    install -m 644 ${WORKDIR}/rotate_emmc ${D}${sysconfdir}/logrotate.d/emmc
  fi

  install -m 644 ${WORKDIR}/rsyslog.logrotate ${D}${sysconfdir}/logrotate.rsyslog
}

FILES_${PN} += "/usr/local/fbpackages/rotate"
