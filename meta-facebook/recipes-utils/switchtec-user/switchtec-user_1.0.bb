SUMMARY = "Switchtec Utility"
DESCRIPTION = "Util for getting information from switch"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
SRCBRANCH = "fb_dev"
SRCREV = "e5cffc5bee689ea2560add51dc1123e771304298"

SRC_URI = "git://github.com/Microsemi/switchtec-user.git;protocol=https;branch=${SRCBRANCH};rev=${SRCREV};downloadfilename=${PN}-${PV}.tar.gz"

# Add version file to hardcode the switchtec version
SRC_URI += "file://0001-Add-version-file-to-hardcode-the-switchtec-version.patch \
           "
LIC_FILES_CHKSUM = "file://LICENSE;beginline=4;endline=16;md5=8de3687db4a8ec31beb8fb79762d30b4"

SRC_URI[md5sum] = "8775fe15632c401b09d37fbb78eaff5a"
SRC_URI[sha256sum] = "83b8805c5f48c105da1847a8d881c795aff79c13fb8429afdb1bbe1ef758b6bf"

S = "${WORKDIR}/git"

do_install() {
    install -d ${D}${bindir}
    install -m 755 switchtec ${D}${bindir}/switchtec
}

DEPENDS += "ncurses"
 
