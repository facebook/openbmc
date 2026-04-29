SUMMARY = "Arduino variant of the BOSSA flashing tool"
HOMEPAGE = "https://github.com/arduino/BOSSA"
SECTION = "devel"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=bcf9399f7b9b96149837290bcdc3ad39"

SRC_URI = "git://github.com/arduino/BOSSA.git;protocol=https;branch=nrf"

PV = "1.9.1+git${SRCPV}"
SRCREV = "89f3556a761833522cd93c199581265ad689310b"

inherit native

do_compile() {
    # We only compile the bossac commandline tool, not the graphical version.
    oe_runmake bossac
}

do_install() {
    install -D -m 0755 ${B}/bin/bossac ${D}${bindir}/bossac
}
