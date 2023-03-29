SUMMARY = "Set of tools to interact with i3c devices from user space"
HOMEPAGE = "https://github.com/vitor-soares-snps/i3c-tools"

LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://i3ctransfer.c;endline=6;md5=8a1ae5c1aaf128e640de497ceaa9935e"

SRC_URI = "git://github.com/AspeedTech-BMC/i3c-tools.git;protocol=https;branch=${BRANCH}"

PR = "r1"
PV = "1.0+git${SRCPV}"
# Tag for v00.01.00
SRCREV = "65f947d74e3a5d33992549a0a1900481bdef95b4"
BRANCH = "master"

S = "${WORKDIR}/git"

inherit meson
