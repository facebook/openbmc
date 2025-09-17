FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0001-Add-delay-before-leak-event-log-to-prevent-AC-OFF-gl.patch \
    file://0002-Implement-valve-monitor-service.patch \
    "
PACKAGECONFIG:append = " valvemonitor"

PACKAGECONFIG[valvemonitor] = "-Dvalve-monitor=enabled, -Dvalve-monitor=disabled"

SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'valvemonitor', \
                                               'xyz.openbmc_project.valvemonitor.service', \
                                               '', d)}"
