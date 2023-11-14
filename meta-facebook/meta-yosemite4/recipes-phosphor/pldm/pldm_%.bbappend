FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

EXTRA_OEMESON:append = " -Dtransport-implementation=af-mctp -Dmaximum-transfer-size=150 -Doem-meta=enabled"

SRC_URI += " \
    file://host_eid \
    file://0001-requester-Modified-MctpDiscovery-class.patch \
    file://0002-requester-Added-coroutine-support-to-send-recv-PLDM-.patch \
    file://0003-platform-mc-Added-TerminusManager-and-Terminus-class.patch \
    file://0004-platform-mc-PDR-handling.patch \
    file://0005-platform-mc-Sensor-handling.patch \
    file://0006-platform-mc-Added-EventManager.patch \
    file://0007-Support-OEM-META-write-file-request-for-post-code-hi.patch \
    file://pldm-restart.sh \
    file://pldm-slow-restart.service \
"

FILES:${PN}:append = " \
    ${systemd_system_unitdir}/pldm-restart.sh \
"

SYSTEMD_SERVICE:${PN} += "pldm-slow-restart.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}/usr/share/pldm
    install -m 0444 ${WORKDIR}/host_eid ${D}/usr/share/pldm
    install -m 0755 ${WORKDIR}/pldm-restart.sh ${D}${datadir}/pldm/
    install -m 0644 ${WORKDIR}/pldm-slow-restart.service ${D}${systemd_system_unitdir}
}

