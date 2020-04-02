SUMMARY = "lm_sensors"
DESCRIPTION = "Hardware health monitoring applications. This package does not include sensord."
HOMEPAGE = "http://www.lm-sensors.org/"
LICENSE = "GPLv2+ & LGPLv2.1+"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe \
                    file://COPYING.LGPL;md5=4fbd65380cdd255951079008b364516c"

DEPENDS = "sysfsutils virtual/libiconv bison-native flex-native"

SRC_URI = "https://github.com/lm-sensors/lm-sensors/archive/V3-4-0.tar.gz \
           file://fancontrol.init \
"
SRC_URI[md5sum] = "1e9f117cbfa11be1955adc96df71eadb"
SRC_URI[sha256sum] = "e334c1c2b06f7290e3e66bdae330a5d36054701ffd47a5dde7a06f9a7402cb4e"

inherit update-rc.d systemd

RDEPENDS_${PN}-dev = ""

INITSCRIPT_PACKAGES = "${PN}-fancontrol"
INITSCRIPT_NAME_${PN}-fancontrol = "fancontrol"
INITSCRIPT_PARAMS_${PN}-fancontrol = "defaults 66"

S = "${WORKDIR}/lm-sensors-3-4-0"

EXTRA_OEMAKE = 'EXLDFLAGS="${LDFLAGS}" \
        MACHINE=${TARGET_ARCH} PREFIX=${prefix} MANDIR=${mandir} \
        LIBDIR=${libdir} \
        CC="${CC}" AR="${AR}"'

do_compile() {
    oe_runmake user PROG_EXTRA="sensors"
}

do_install() {
    oe_runmake user_install DESTDIR=${D}

    # Install directory
    install -d ${D}${sysconfdir}/init.d

    # Install fancontrol init script
    install -m 0755 ${WORKDIR}/fancontrol.init \
        ${D}${sysconfdir}/init.d/fancontrol
}

# libsensors packages
PACKAGES =+ "${PN}-libsensors ${PN}-libsensors-dbg ${PN}-libsensors-dev ${PN}-libsensors-staticdev ${PN}-libsensors-doc"

# sensors command packages
PACKAGES =+ "${PN}-sensors ${PN}-sensors-dbg ${PN}-sensors-doc"

# fancontrol script
PACKAGES =+ "${PN}-fancontrol ${PN}-fancontrol-doc"

# sensors-detect script
PACKAGES =+ "${PN}-sensorsdetect ${PN}-sensorsdetect-doc"

# sensors-conf-convert script
PACKAGES =+ "${PN}-sensorsconfconvert ${PN}-sensorsconfconvert-doc"

# pwmconfig script
PACKAGES =+ "${PN}-pwmconfig ${PN}-pwmconfig-doc"

# isadump and isaset helper program
PACKAGES =+ "${PN}-isatools ${PN}-isatools-dbg ${PN}-isatools-doc"

# libsensors files
FILES_${PN}-libsensors = "${libdir}/libsensors.so.* ${sysconfdir}/sensors3.conf ${sysconfdir}/sensors.d"
FILES_${PN}-libsensors-dbg = "${libdir}/.debug ${prefix}/src/debug"
FILES_${PN}-libsensors-dev = "${libdir}/libsensors.so ${includedir}"
FILES_${PN}-libsensors-staticdev = "${libdir}/libsensors.a"
FILES_${PN}-libsensors-doc = "${mandir}/man3"
RRECOMMENDS_${PN}-libsensors = "lmsensors-config-libsensors"

# sensors command files
FILES_${PN}-sensors = "${bindir}/sensors"
FILES_${PN}-sensors-dbg = "${bindir}/.debug/sensors"
FILES_${PN}-sensors-doc = "${mandir}/man1 ${mandir}/man5"
RDEPENDS_${PN}-sensors = "${PN}-libsensors"

# fancontrol script files
FILES_${PN}-fancontrol = "${sbindir}/fancontrol ${sysconfdir}/init.d/fancontrol"
FILES_${PN}-fancontrol-doc = "${mandir}/man8/fancontrol.8"
RDEPENDS_${PN}-fancontrol = "bash"
RRECOMMENDS_${PN}-fancontrol = "lmsensors-config-fancontrol"

# sensors-detect script files
FILES_${PN}-sensorsdetect = "${sbindir}/sensors-detect"
FILES_${PN}-sensorsdetect-doc = "${mandir}/man8/sensors-detect.8"
RDEPENDS_${PN}-sensorsdetect = "${PN}-sensors perl perl-modules"

# sensors-conf-convert script files
FILES_${PN}-sensorsconfconvert = "${bindir}/sensors-conf-convert"
FILES_${PN}-sensorsconfconvert-doc = "${mandir}/man8/sensors-conf-convert.8"
RDEPENDS_${PN}-sensorsconfconvert = "${PN}-sensors perl perl-modules"

# pwmconfig script files
FILES_${PN}-pwmconfig = "${sbindir}/pwmconfig"
FILES_${PN}-pwmconfig-doc = "${mandir}/man8/pwmconfig.8"
RDEPENDS_${PN}-pwmconfig = "${PN}-fancontrol bash"

# isadump and isaset helper program files
FILES_${PN}-isatools = "${sbindir}/isa*"
FILES_${PN}-isatools-dbg = "${sbindir}/.debug/isa*"
FILES_${PN}-isatools-doc = "${mandir}/man8/isa*"
