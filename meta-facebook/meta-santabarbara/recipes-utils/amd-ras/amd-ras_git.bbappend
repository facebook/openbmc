FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

inherit obmc-phosphor-systemd

SRC_URI:append = " \
    file://amd-ras-overrides.conf \
    file://ras_config.json \
    "

SYSTEMD_OVERRIDE:${PN}:append = " \
    amd-ras-overrides.conf:com.amd.RAS@1.service.d/amd-ras-overrides.conf \
    "

do_install:append() {
    install -d ${D}/${datadir}/amd-bmc-ras
    install -m 0644 ${UNPACKDIR}/ras_config.json ${D}/${datadir}/amd-bmc-ras/ras_config.json
}

SYSTEMD_LINK:${PN}:append = " ../com.amd.RAS@.service:multi-user.target.wants/com.amd.RAS@1.service"
