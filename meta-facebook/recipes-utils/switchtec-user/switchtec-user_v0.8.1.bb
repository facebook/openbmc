SUMMARY = "Switchtec Utility"
DESCRIPTION = "Util for getting information from switch"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
SRCBRANCH = "fb_dev"

SRC_URI = "https://github.com/Microsemi/switchtec-user.git;branch=${SRCBRANCH};tag=v0.8+twi-special;downloadfilename=${PN}-${PV}.tar.gz"
LIC_FILES_CHKSUM = "file://LICENSE;beginline=4;endline=16;md5=8de3687db4a8ec31beb8fb79762d30b4"

SRC_URI[md5sum] = "8775fe15632c401b09d37fbb78eaff5a"
SRC_URI[sha256sum] = "83b8805c5f48c105da1847a8d881c795aff79c13fb8429afdb1bbe1ef758b6bf"

S = "${WORKDIR}/switchtec-user-0.8-twi-special/"

do_install() {
 
    install -d ${D}${bindir}
    install -m 755 switchtec ${D}${bindir}/switchtec
 
    install -d ${D}${libdir}
    install -m 0644 libswitchtec.so ${D}${libdir}/libswitchtec.so.5
 
    install -d ${D}${libdir}
    install -m 0644 libswitchtec.a ${D}${libdir}/libswitchtec.a
}
 
DEPENDS += "ncurses"
FILES_${PN} = "${bindir} ${libdir}/libswitchtec.so.5 ${libdir}/libswitchtec.a"
 
INHIBIT_PACKAGE_STRIP = "1"
