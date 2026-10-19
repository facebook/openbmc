FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://wedge-host-poweron@.service \
    file://wedge-host-poweroff@.service \
    file://wedge-host-powerreset@.service \
    file://phosphor-wait-power-off@.service \
    file://0001-fix-phosphor-state-manager-build-error-add-gpiodcxx.patch \
"

RDEPENDS:${PN} += " bash "
EXTRA_OEMESON:append = " -Dwarm-reboot=enabled"

do_install:append () {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/*.service ${D}${systemd_system_unitdir}
    install -d ${D}${libexecdir}/${PN}
}

FILES:${PN}:append = " ${systemd_system_unitdir}"
FILES:${PN}:append = " ${libexecdir}/${PN}"

HOST_DEFAULT_TARGETS:remove = " \
    obmc-host-reboot@{}.target.requires/obmc-host-shutdown@{}.target \
    obmc-host-reboot@{}.target.requires/phosphor-reboot-host@{}.service \
    obmc-host-force-warm-reboot@{}.target.requires/obmc-host-stop@{}.target \
    obmc-host-force-warm-reboot@{}.target.requires/phosphor-reboot-host@{}.service \
"

CHASSIS_DEFAULT_TARGETS:remove = " \
    obmc-chassis-powerreset@{}.target.requires/phosphor-reset-chassis-on@{}.service \
    obmc-chassis-powerreset@{}.target.requires/phosphor-reset-chassis-running@{}.service \
    obmc-chassis-poweron@{}.target.requires/phosphor-reset-chassis-on@{}.service \
"

HOST_DEFAULT_TARGETS:append = " \
    obmc-host-start@{}.target.wants/wedge-host-poweron@{}.service \
    obmc-host-stop@{}.target.wants/wedge-host-poweroff@{}.service \
    obmc-host-stop@{}.target.wants/phosphor-wait-power-off@{}.service \
    obmc-host-reboot@{}.target.wants/wedge-host-powerreset@{}.service \
    obmc-host-force-warm-reboot@{}.target.wants/wedge-host-powerreset@{}.service \
"

CHASSIS_DEFAULT_TARGETS:append = " \
    obmc-chassis-poweron@{}.target.wants/wedge-host-poweron@{}.service \
"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = " \
    wedge-host-poweron@.service \
    wedge-host-poweroff@.service \
    wedge-host-powerreset@.service \
    phosphor-wait-power-off@.service \
"