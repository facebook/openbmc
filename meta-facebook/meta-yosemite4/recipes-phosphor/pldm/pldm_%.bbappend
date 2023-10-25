FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

EXTRA_OEMESON:append = " -Dtransport-implementation=af-mctp -Dmaximum-transfer-size=150"

SRC_URI += " \
    file://mctp-bridge.conf \
    file://host_eid \
    file://0001-requester-Modified-MctpDiscovery-class.patch \
    file://0002-requester-Added-coroutine-support-to-send-recv-PLDM-.patch \
    file://0003-platform-mc-Added-TerminusManager-and-Terminus-class.patch \
    file://0004-platform-mc-PDR-handling.patch \
    file://0005-platform-mc-Sensor-handling.patch \
    file://0006-platform-mc-Added-EventManager.patch \
"

FILES:${PN}:append = " \
    ${systemd_system_unitdir}/pldmd.service.d/mctp-bridge.conf \
"

do_install:append() {
    override_dir=${D}${systemd_system_unitdir}/pldmd.service.d
    install -d ${override_dir}
    install -m 0644 ${WORKDIR}/mctp-bridge.conf ${override_dir}/mctp-bridge.conf
    install -d ${D}/usr/share/pldm
    install -m 0444 ${WORKDIR}/host_eid ${D}/usr/share/pldm
}

