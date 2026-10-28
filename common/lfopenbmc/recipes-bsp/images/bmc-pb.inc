SUMMARY = "ASPEED BMC Prebuilt Binaries"
HOMEPAGE = "https://github.com/AspeedTech-BMC/bmc-pb"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

BRANCH = "master"
SRC_URI = "git://github.com/AspeedTech-BMC/bmc-pb.git;protocol=https;branch=${BRANCH}"

# Tag for v00.01.03
SRCREV = "0991441478c26407ec949fcd8bf1f51b7cff94c0"

PV = "1.0+git"
