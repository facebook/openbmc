FILESEXTRAPATHS:prepend:openbmc-fb := "${THISDIR}/files:"

FILES:${PN}:append:openbmc-fb = " ${systemd_system_unitdir}/*.service"

S:openbmc-fb = "${WORKDIR}/sources"
UNPACKDIR:openbmc-fb = "${S}"
SRC_URI:openbmc-fb = "file://serial-getty@.service"
SERIAL_TERM:openbmc-fb ?= "linux"

do_install:prepend:openbmc-fb() {
    if [ ! -z "${SERIAL_CONSOLES}" ] ; then
        default_baudrate=`echo "${SERIAL_CONSOLES}" | sed 's/\;.*//'`
        install -d ${D}${systemd_system_unitdir}/
        install -d ${D}${sysconfdir}/systemd/system/getty.target.wants/
        install -m 0644 ${S}/serial-getty@.service ${D}${systemd_system_unitdir}/
        sed -i -e "s/\@BAUDRATE\@/$default_baudrate/g" ${D}${systemd_system_unitdir}/serial-getty@.service
        sed -i -e "s/\@TERM\@/${SERIAL_TERM}/g" ${D}${systemd_system_unitdir}/serial-getty@.service
    fi
}
