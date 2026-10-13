FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

inherit obmc-phosphor-dbus-service

SYSTEMD_OVERRIDE:${PN}:append = "\
    wait-scm-inventory.conf:xyz.openbmc_project.RedfishClient.service.d/wait-scm-inventory.conf \
"
