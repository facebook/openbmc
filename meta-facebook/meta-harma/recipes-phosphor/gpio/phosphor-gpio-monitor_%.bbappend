FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

inherit obmc-phosphor-systemd systemd

SYSTEMD_OVERRIDE:${PN}-monitor += "mmc-recovery.conf:mmc-recovery.service.d/mmc-recovery.conf"
