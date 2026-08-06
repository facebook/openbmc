CFLAGS:prepend = "-DCONFIG_GRANDCANYON2"


FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append:grandcanyon2 = " file://clear-hsc-fault.sh"

do_install:append:grandcanyon2() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${UNPACKDIR}/clear-hsc-fault.sh ${D}${sysconfdir}/init.d/clear-hsc-fault.sh
    update-rc.d -r ${D} clear-hsc-fault.sh start 89 5 .
}