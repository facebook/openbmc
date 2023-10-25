FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://mctp-bridge.conf \
"

FILES:${PN}:append = " \
    ${systemd_system_unitdir}/pldmd.service.d/mctp-bridge.conf \
"

do_install:append() {
    override_dir=${D}${systemd_system_unitdir}/pldmd.service.d
    install -d ${override_dir}
    install -m 0644 ${WORKDIR}/mctp-bridge.conf ${override_dir}/mctp-bridge.conf
}

