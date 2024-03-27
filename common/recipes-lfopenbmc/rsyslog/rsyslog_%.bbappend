FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/files:"

SRC_URI:append:openbmc-fb-lf = "\
    file://rsyslog-remote.conf.in \
    "

do_install:append:openbmc-fb-lf() {
  rsysconf="${D}${sysconfdir}/rsyslog.d"
  install -d ${rsysconf}
  remote_conf="${rsysconf}/remote.conf"
  install -m 644 ${WORKDIR}/rsyslog-remote.conf.in ${remote_conf}
  sed -i "s/__OPENBMC_VERSION__/${OPENBMC_VERSION}/g" ${remote_conf}
}
