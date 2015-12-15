FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
            file://busybox.cfg \
            file://syslog.conf \
           "
