FILESEXTRAPATHS:prepend := "${THISDIR}/files:"


SRC_URI += " \
    file://0500-Support-get-static-eid-config-from-entity-manager.patch \
    file://0501-Modified-the-type-of-NetworkId-to-uint32_t.patch \
    file://setup-local-eid.conf \
    file://setup-local-eid.service \
    file://mctp-config.sh \
"

FILES:${PN} += "${systemd_system_unitdir}/*"

SYSTEMD_SERVICE:${PN} += "setup-local-eid.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append () {
    override_dir=${D}${systemd_system_unitdir}/mctpd.service.d
    install -d ${override_dir}
    install -d ${D}${systemd_system_unitdir}
    install -d ${D}${datadir}/mctp
    install -m 0644 ${WORKDIR}/setup-local-eid.conf \
            ${override_dir}/mctp-bridge.conf
    install -m 0644 ${WORKDIR}/setup-local-eid.service \
            ${D}${systemd_system_unitdir}
    install -m 0755 ${WORKDIR}/mctp-config.sh \
            ${D}${datadir}/mctp/
}
