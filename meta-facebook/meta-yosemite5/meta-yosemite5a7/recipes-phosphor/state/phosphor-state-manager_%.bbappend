FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

HOST_DEFAULT_TARGETS:append = " \
    obmc-host-startmin@{}.target.requires/drive-hoston.service \
    obmc-host-stop@{}.target.requires/drive-hostoff.service \
"

SRC_URI += "file://drive-poweron@.service \
            file://drive-poweroff@.service \
            file://drive-powercycle@.service \
            file://drive-reboot@.service \
            file://drive-hoston.service \
            file://drive-hostoff.service \
            file://drive-poweron \
            file://drive-poweroff \
            file://drive-powercycle \
            file://drive-host-on \
            file://drive-host-off \
            file://drive-reboot \
            file://drive-power-control-util \
            "


SYSTEMD_SERVICE:${PN} += " drive-poweron@.service \
                           drive-poweroff@.service \
                           drive-powercycle@.service \
                           drive-reboot@.service \
                           drive-hoston.service \
                           drive-hostoff.service \
"

FILES:${PN} += " \
    ${systemd_system_unitdir}/obmc-drive-poweron@.target.requires \
    ${systemd_system_unitdir}/obmc-drive-powercycle@.target.requires \
    ${systemd_system_unitdir}/obmc-drive-poweroff@.target.requires \
    ${systemd_system_unitdir}/obmc-drive-reboot@.target.requires \
    ${systemd_system_unitdir}/obmc-drive-poweron@.target.requires/* \
    ${systemd_system_unitdir}/obmc-drive-powercycle@.target.requires/* \
    ${systemd_system_unitdir}/obmc-drive-poweroff@.target.requires/* \
    ${systemd_system_unitdir}/obmc-drive-reboot@.target.requires/* \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -d ${D}${libexecdir}/${PN}

    install -d ${D}${systemd_system_unitdir}/obmc-drive-poweron@.target.requires
    install -d ${D}${systemd_system_unitdir}/obmc-drive-poweroff@.target.requires
    install -d ${D}${systemd_system_unitdir}/obmc-drive-powercycle@.target.requires
    install -d ${D}${systemd_system_unitdir}/obmc-drive-reboot@.target.requires

    ln -s ../../drive-poweron@.service ${D}${systemd_system_unitdir}/obmc-drive-poweron@.target.requires/drive-poweron@.service
    ln -s ../../drive-poweroff@.service ${D}${systemd_system_unitdir}/obmc-drive-poweroff@.target.requires/drive-poweroff@.service
    ln -s ../../drive-powercycle@.service ${D}${systemd_system_unitdir}/obmc-drive-powercycle@.target.requires/drive-powercycle@.service
    ln -s ../../drive-reboot@.service ${D}${systemd_system_unitdir}/obmc-drive-reboot@.target.requires/drive-reboot@.service

    install -m 0644 ${UNPACKDIR}/drive-poweron@.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${UNPACKDIR}/drive-poweroff@.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${UNPACKDIR}/drive-powercycle@.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${UNPACKDIR}/drive-reboot@.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${UNPACKDIR}/drive-hoston.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${UNPACKDIR}/drive-hostoff.service ${D}${systemd_system_unitdir}/
    install -m 0755 ${UNPACKDIR}/drive-poweron ${D}${libexecdir}/${PN}/
    install -m 0755 ${UNPACKDIR}/drive-poweroff ${D}${libexecdir}/${PN}/
    install -m 0755 ${UNPACKDIR}/drive-powercycle ${D}${libexecdir}/${PN}/
    install -m 0755 ${UNPACKDIR}/drive-power-control-util ${D}${libexecdir}/${PN}/
    install -m 0755 ${UNPACKDIR}/drive-host-on ${D}${libexecdir}/${PN}/
    install -m 0755 ${UNPACKDIR}/drive-host-off ${D}${libexecdir}/${PN}/
    install -m 0755 ${UNPACKDIR}/drive-reboot ${D}${libexecdir}/${PN}/
}
