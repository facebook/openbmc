FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:openbmc-fb = " \
            file://fb-busybox.cfg \
            file://setup_crond.service \
            file://setup_crond.sh \
            file://cron.daily/biosfwimages-cleanup.sh \
           "

DEPENDS:append:openbmc-fb = " update-rc.d-native"

inherit systemd

FILES:${PN} += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_system_unitdir} /usr/local/bin ', '', d)}"

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -m 755 ../setup_crond.sh ${D}${sysconfdir}/init.d/setup_crond.sh
  update-rc.d -r ${D} setup_crond.sh start 97 5 .
}

install_systemd() {
  install -d ${D}${systemd_system_unitdir}
  install -d ${D}${prefix}/local/bin
  install -m 755 ../setup_crond.sh ${D}${prefix}/local/bin/setup_crond.sh
  install -m 644 ../setup_crond.service ${D}${systemd_system_unitdir}/setup_crond.service
  # XXX hack: SYSTEMD_SERVICE not functioning in this recipe
  systemctl --root=${D} enable setup_crond.service
}

do_install:append:openbmc-fb() {
  install -d ${D}${sysconfdir}/cron.daily
  install -m 755 ../cron.daily/biosfwimages-cleanup.sh ${D}${sysconfdir}/cron.daily/biosfwimages-cleanup.sh

  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
    install_systemd
  else
    install_sysv
  fi
}

SYSTEMD_SERVICE:${PN} = "setup_crond.service"
