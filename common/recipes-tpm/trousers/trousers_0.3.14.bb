SUMMARY = "TrouSerS - An open-source TCG Software Stack implementation."
LICENSE = "BSD"
HOMEPAGE = "http://sourceforge.net/projects/trousers/"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8031b2ae48ededc9b982c08620573426"
SECTION = "security/tpm"

DEPENDS = "openssl"

SRC_URI = "http://sourceforge.net/projects/trousers/files/${BPN}/${PV}/${BPN}-${PV}.tar.gz;subdir=${BPN}-${PV} \
    file://trousers.init.sh \
    file://trousers-udev.rules \
    file://tcsd.service \
    "

SRC_URI[md5sum] = "4a476b4f036dd20a764fb54fc24edbec"
SRC_URI[sha256sum] = "ce50713a261d14b735ec9ccd97609f0ad5ce69540af560e8c3ce9eb5f2d28f47"

inherit autotools pkgconfig useradd update-rc.d ${@bb.utils.contains('VIRTUAL-RUNTIME_init_manager','systemd','systemd','', d)}

PACKAGECONFIG ?= "gmp "
PACKAGECONFIG[gmp] = "--with-gmp, --with-gmp=no, gmp"
PACKAGECONFIG[gtk] = "--with-gui=gtk, --with-gui=none, gtk+"

do_install () {
    oe_runmake DESTDIR=${D} install
}

do_install_append() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/trousers.init.sh ${D}${sysconfdir}/init.d/trousers
    install -d ${D}${sysconfdir}/udev/rules.d
    install -m 0644 ${WORKDIR}/trousers-udev.rules ${D}${sysconfdir}/udev/rules.d/45-trousers.rules

    if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
        install -d ${D}${systemd_unitdir}/system
        install -m 0644 ${WORKDIR}/tcsd.service ${D}${systemd_unitdir}/system/
        sed -i -e 's#@SBINDIR@#${sbindir}#g' ${D}${systemd_unitdir}/system/tcsd.service
    fi
    chown -R tss:tss ${D}${sysconfdir}/tcsd.conf
}

CONFFILES_${PN} += "${sysconfig}/tcsd.conf"

PROVIDES = "${PACKAGES}"
PACKAGES = " \
	libtspi \
	libtspi-dbg \
	libtspi-dev \
	libtspi-doc \
	libtspi-staticdev \
	trousers \
	trousers-dbg \
	trousers-doc \
	"

FILES_libtspi = " \
	${libdir}/*.so.1.2.0 \
	"
FILES_libtspi-dbg = " \
	${libdir}/.debug \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/tspi \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/trspi \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/include/*.h \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/include/tss \
	"
FILES_libtspi-dev = " \
	${includedir} \
	${libdir}/*.so \
	${libdir}/*.so.1 \
	"
FILES_libtspi-doc = " \
	${mandir}/man3 \
	"
FILES_libtspi-staticdev = " \
	${libdir}/*.la \
	${libdir}/*.a \
	"
FILES_${PN} = " \
	${sbindir}/tcsd \
	${sysconfdir} \
	${localstatedir} \
	"

FILES_${PN}-dev += "${libdir}/trousers"

FILES_${PN}-dbg = " \
	${sbindir}/.debug \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/tcs \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/tcsd \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/tddl \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/trousers \
	${prefix}/src/debug/${PN}/${PV}-${PR}/${PN}-${PV}/src/include/trousers \
	"
FILES_${PN}-doc = " \
	${mandir}/man5 \
	${mandir}/man8 \
	"

INITSCRIPT_NAME = "trousers"
INITSCRIPT_PARAMS = "start 99 2 3 4 5 . stop 19 0 1 6 ."

USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM_${PN} = "tss"
USERADD_PARAM_${PN} = "-M -d /var/lib/tpm -s /bin/false -g tss tss"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "tcsd.service"
SYSTEMD_AUTO_ENABLE = "disable"

BBCLASSEXTEND = "native"
