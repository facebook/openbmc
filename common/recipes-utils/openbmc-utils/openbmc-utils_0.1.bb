# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

SUMMARY = "OpenBMC Utilities"
DESCRIPTION = "Various OpenBMC utilities"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

PACKAGECONFIG ??= ""
PACKAGECONFIG[disable-watchdog] = ""

SRC_URI = " \
    file://COPYING \
    file://mount_data0.sh \
    file://openbmc-utils.sh \
    file://shell-utils.sh \
    file://i2c-utils.sh \
    file://gpio-utils.sh \
    file://rc.early \
    file://rc.local \
    file://dhclient-exit-hooks \
    file://rm_poweroff_cmd.sh \
    file://rm_poweroff_cmd.service \
    file://revise_inittab \
    file://blkdev_mount.sh \
    file://emmc_auto_mount.sh \
    file://emmc_enhance_part.sh \
    file://mount_data1.sh \
    file://setup_persist_log.sh \
    file://setup-reboot.sh \
    file://setup-reboot.service \
    file://eth0_mac_fixup.sh \
    file://create_vlan_intf \
    ${@bb.utils.contains('PACKAGECONFIG', 'disable-watchdog', \
                         'file://disable_watchdog.sh ' + \
                         'file://disable_watchdog.service', '', d)} \
    "

SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', \
                                 'file://eth0_mac_fixup.sh', '', d)}"

OPENBMC_UTILS_FILES = " \
    mount_data0.sh \
    openbmc-utils.sh \
    shell-utils.sh \
    i2c-utils.sh \
    gpio-utils.sh \
    rc.early \
    rc.local \
    rm_poweroff_cmd.sh \
    revise_inittab \
    blkdev_mount.sh \
    emmc_auto_mount.sh \
    emmc_enhance_part.sh \
    "

S = "${WORKDIR}"

inherit systemd

DEPENDS = "update-rc.d-native"
RDEPENDS_${PN} += "bash"

OPENBMC_UTILS_CUSTOM_EMMC_MOUNT ?= "0"

install_sysv() {
    pkgdir="/usr/local/packages/utils"
    dstdir="${D}${pkgdir}"

    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d

    # the script to process dhcp options
    install -m 0755 ${WORKDIR}/dhclient-exit-hooks ${D}${sysconfdir}/dhclient-exit-hooks

    # the script to mount /mnt/data, after udev (level 4) is started
    install -m 0755 ${WORKDIR}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
    update-rc.d -r ${D} mount_data0.sh start 05 S .

    install -m 0755 ${WORKDIR}/rc.early ${D}${sysconfdir}/init.d/rc.early
    update-rc.d -r ${D} rc.early start 06 S .

    install -m 0755 ${WORKDIR}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 0755 ${WORKDIR}/rm_poweroff_cmd.sh ${D}${sysconfdir}/init.d/rm_poweroff_cmd.sh
    update-rc.d -r ${D} rm_poweroff_cmd.sh start 99 S .

    install -m 755 ${WORKDIR}/setup-reboot.sh ${D}${sysconfdir}/init.d/setup-reboot.sh
    update-rc.d -r ${D} setup-reboot.sh start 89 6 .

    if ! echo ${MACHINE_FEATURES} | awk "/emmc/ {exit 1}"; then
        if [ "x${OPENBMC_UTILS_CUSTOM_EMMC_MOUNT}" = "x0" ]; then
            # auto-mount emmc to /mnt/data1
            install -m 0755 ${WORKDIR}/mount_data1.sh \
                    ${D}${sysconfdir}/init.d/mount_data1.sh
            update-rc.d -r ${D} mount_data1.sh start 05 S .
        fi
    fi

    # If emmc-ext4 feature is enabled, we want to default to ext4 over btrfs.
    # Update the blkdev_mount script to reflect this.
    if ! echo ${MACHINE_FEATURES} | awk "/emmc-ext4/ {exit 1}"; then
        sed -i 's/="btrfs"/="ext4"/' ${dstdir}/blkdev_mount.sh
    fi

    # Install disable-watchdog.
    if ! echo ${PACKAGECONFIG} | awk "/disable-watchdog/ {exit 1}"; then
        install -m 0755 ${S}/disable_watchdog.sh \
                        ${D}${sysconfdir}/init.d/disable_watchdog.sh
        update-rc.d -r ${D} disable_watchdog.sh start 99 2 3 4 5 .
    fi
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 644 ${WORKDIR}/eth0_mac_fixup.sh ${D}/usr/local/bin
    install -m 755 ${WORKDIR}/setup-reboot.sh ${D}/usr/local/bin
    install -m 755 ${WORKDIR}/rc.local ${D}/usr/local/bin
    install -m 755 ${WORKDIR}/rc.early ${D}/usr/local/bin
    install -m 644 ${WORKDIR}/early.service ${D}${systemd_system_unitdir}
    install -m 644 ${WORKDIR}/rm_poweroff_cmd.service ${D}${systemd_system_unitdir}
    # No rm_poweroff_cmd.sh under systemd
    install -m 644 ${WORKDIR}/setup-reboot.service ${D}${systemd_system_unitdir}
    # data1 will be mounted via fstab in a different recipe

    # Install disable-watchdog.
    if ! echo ${PACKAGECONFIG} | awk "/disable-watchdog/ {exit 1}"; then
        install -m 0755 ${S}/disable_watchdog.sh ${D}/usr/local/bin
        install -m 0644 ${S}/disable_watchdog.service \
                        ${D}${systemd_system_unitdir}
    fi
}

do_install() {
    pkgdir="/usr/local/packages/utils"
    dstdir="${D}${pkgdir}"
    install -d $dstdir
    localbindir="${D}/usr/local/bin"
    install -d "${D}${sysconfdir}"
    install -d ${localbindir}

    # If mtd-ubifs feature is enabled, we want ubifs on top of the mtd
    # data0 partition. Update the "mount_data0.sh" to reflect this.
    if ! echo ${MACHINE_FEATURES} | awk "/mtd-ubifs/ {exit 1}"; then
        sed -i 's/FLASH_FS_TYPE=jffs2/FLASH_FS_TYPE=ubifs/' ${WORKDIR}/mount_data0.sh
    fi

    for f in ${OPENBMC_UTILS_FILES}; do
        install -m 755 $f ${dstdir}/${f}
        ln -s ${pkgdir}/${f} ${localbindir}
    done

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi

}

FILES_${PN} += "/usr/local"

SYSTEMD_SERVICE_${PN} = " \
    early.service \
    fetch-backports.service \
    rm_poweroff_cmd.service \
    setup-reboot.service \
    ${@bb.utils.contains('PACKAGECONFIG', 'disable-watchdog', \
                         'disable_watchdog.service', '', d)} \
    "
