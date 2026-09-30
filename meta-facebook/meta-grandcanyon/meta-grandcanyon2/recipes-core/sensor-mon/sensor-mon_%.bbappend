FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

GC2_SENSOR_MON_FILESDIR := "${THISDIR}/files"

do_install:append() {
    install -d ${D}/etc/sensord
    install -m 0644 ${GC2_SENSOR_MON_FILESDIR}/power_transition_filter.json \
                    ${D}/etc/sensord/power_transition_filter.json
}

FILES:${PN} += "/etc/sensord/power_transition_filter.json"