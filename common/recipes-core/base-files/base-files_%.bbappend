FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://aliases.sh \
    file://console_idle_logout.sh \
"

# Enable the idle timeout for auto logout on console port.
# Change the value to 0 will disable this feature.
CONSOLE_IDLE_TIMEOUT ?= "300"

do_install_append() {
    install -d ${D}/${sysconfdir}/profile.d
    install -m 644 aliases.sh ${D}/${sysconfdir}/profile.d/aliases.sh
    if [[ ${CONSOLE_IDLE_TIMEOUT} -ne 0 ]]; then
        install -m 644 console_idle_logout.sh ${D}/${sysconfdir}/profile.d/console_idle_logout.sh
        sed -i 's/__TMOUT__/${CONSOLE_IDLE_TIMEOUT}/g' ${D}/${sysconfdir}/profile.d/console_idle_logout.sh
    fi
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false',  d)}; then
        cat <<EOF >> ${D}/${sysconfdir}/fstab
/dev/data0   /mnt/data   jffs2  defaults,nofail,x-systemd.device-timeout=1,x-systemd.makefs
EOF
        if ${@bb.utils.contains('MACHINE_FEATURES', 'emmc-ext4', 'true', 'false', d)}; then
            cat <<EOF >> ${D}/${sysconfdir}/fstab
/dev/mmcblk0    /mnt/data1      ext4    defaults,nofail,x-systemd.device-timeout=1,x-systemd.makefs
EOF
        elif ${@bb.utils.contains('MACHINE_FEATURES', 'emmc', 'true', 'false', d)}; then
             cat <<EOF >> ${D}/${sysconfdir}/fstab
/dev/mmcblk0    /mnt/data1      btrfs    defaults,nofail,x-systemd.device-timeout=1,x-systemd.makefs
EOF
        fi
    fi
}
