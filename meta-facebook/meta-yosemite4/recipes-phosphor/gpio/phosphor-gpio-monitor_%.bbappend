FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

inherit obmc-phosphor-systemd systemd

SRC_URI += "file://yosemite4-phosphor-multi-gpio-monitor.json \
            file://slot-plug-in@.service \
            file://slot-plug-in \
            file://slot-unplug@.service \
            file://slot-unplug \
            "

RDEPENDS:${PN}:append = " bash"

FILES:${PN} += "${systemd_system_unitdir}/*"

SYSTEMD_SERVICE:${PN} += " \
    slot-plug-in@.service \
    slot-unplug@.service \
    "

SYSTEMD_AUTO_ENABLE = "enable"

do_install:append:() {
    install -m 0644 ${WORKDIR}/yosemite4-phosphor-multi-gpio-monitor.json \
                    ${D}${datadir}/phosphor-gpio-monitor/phosphor-multi-gpio-monitor.json
    install -m 0644 ${WORKDIR}/slot-plug-in@.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/slot-unplug@.service ${D}${systemd_system_unitdir}/
    install -m 0755 ${WORKDIR}/slot-plug-in ${D}${libexecdir}/${PN}/
    install -m 0755 ${WORKDIR}/slot-unplug ${D}${libexecdir}/${PN}/
}

