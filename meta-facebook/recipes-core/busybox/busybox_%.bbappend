FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:openbmc-fb = " \
            file://busybox.cfg \
            file://setup_crond.sh \
            file://cron.daily/biosfwimages-cleanup.sh \
           "

DEPENDS:append:openbmc-fb = " update-rc.d-native"

do_install:append:openbmc-fb() {
  install -d ${D}${sysconfdir}/cron.daily
  install -m 755 ../cron.daily/biosfwimages-cleanup.sh ${D}${sysconfdir}/cron.daily/biosfwimages-cleanup.sh
  install -d ${D}${sysconfdir}/init.d
  install -m 755 ../setup_crond.sh ${D}${sysconfdir}/init.d/setup_crond.sh
  update-rc.d -r ${D} setup_crond.sh start 97 5 .
}
