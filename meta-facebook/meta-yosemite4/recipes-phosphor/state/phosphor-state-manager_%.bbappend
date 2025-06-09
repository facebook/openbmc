FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://phosphor-reset-host-reboot-attempts_override.conf \
    file://1000-Add-retry-mechanism-with-configurable-Meson-option.patch \
"
EXTRA_OEMESON:append = " \
    -Dretry_attempts=5 \
    -Donly-allow-boot-when-bmc-ready=true \
"

do_install:append() {
    override_dir=${D}${systemd_system_unitdir}/phosphor-reset-host-reboot-attempts@.service.d
    install -d ${override_dir}
    install -m 0644 ${UNPACKDIR}/phosphor-reset-host-reboot-attempts_override.conf \
            ${override_dir}/phosphor-reset-host-reboot-attempts_override.conf
}

FILES:${PN} += "${systemd_system_unitdir}/phosphor-reset-host-reboot-attempts@.service.d/phosphor-reset-host-reboot-attempts_override.conf"
