SUMMARY = "TrouSerS - An open-source TCG Software Stack implementation."
LICENSE = "BSD"
HOMEPAGE = "http://sourceforge.net/projects/trousers/"
LIC_FILES_CHKSUM = "file://README;startline=3;endline=4;md5=2af28fbed0832e4d83a9e6dd68bb4413"
SECTION = "security/tpm"

DEPENDS = "openssl"

SRC_URI = "http://sourceforge.net/projects/trousers/files/${BPN}/${PV}/${BPN}-${PV}.tar.gz \
    file://07-read_data-not-inline.patch \
    file://trousers.init.sh \
    file://trousers-udev.rules \
    file://tcsd.service \
    "

SRC_URI[md5sum] = "ad508f97b406f6e48cd90e85d78e7ca8"
SRC_URI[sha256sum] = "bb908e4a3c88a17b247a4fc8e0fff3419d8a13170fe7bdfbe0e2c5c082a276d3"

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
