# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Health Monitoring Daemon"
DESCRIPTION = "Daemon for BMC Health monitoring"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://healthd.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://Makefile \
           file://healthd.c \
           file://pfr_monitor.c \
           file://setup-healthd.sh \
           file://run-healthd.sh \
           file://healthd-config.json \
           file://healthd.service \
          "
S = "${WORKDIR}"

inherit systemd

LDFLAGS =+ " -lpal -ljansson -lkv -lwatchdog -lvbs -lobmc-i2c -lmisc-utils "

DEPENDS =+ " libpal jansson libobmc-i2c libkv update-rc.d-native libwatchdog libvbs libmisc-utils"

binfiles = "healthd"

pkgdir = "healthd"

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 644 healthd.service ${D}${systemd_system_unitdir}
}

install_sysv() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -d ${D}${sysconfdir}/sv
    install -d ${D}${sysconfdir}/sv/healthd
    install -d ${D}${sysconfdir}/healthd
    install -m 755 setup-healthd.sh ${D}${sysconfdir}/init.d/setup-healthd.sh
    install -m 755 run-healthd.sh ${D}${sysconfdir}/sv/healthd/run
    update-rc.d -r ${D} setup-healthd.sh start 91 5 .
}

do_install() {
    dst="${D}/usr/local/fbpackages/${pkgdir}"
    bin="${D}/usr/local/bin"
    sysconf="${D}${sysconfdir}"
    install -d $dst
    install -d $bin
    install -d $sysconf
    install -m 755 healthd ${dst}/healthd
    ln -snf ../fbpackages/${pkgdir}/healthd ${bin}/healthd

    install -m 644 healthd-config.json $sysconf/healthd-config.json

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi

}

RDEPENDS_${PN} =+ " libpal jansson libwatchdog libvbs libobmc-i2c libmisc-utils "

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/healthd ${prefix}/local/bin ${sysconfdir} "

SYSTEMD_SERVICE_${PN} = "healthd.service"
