SUMMARY = "Generate CPTRA authorization flash image with cptra_imgtool"
HOMEPAGE = "https://github.com/AspeedTech-BMC/cptra_imgtool"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

BRANCH = "master"
SRC_URI = "git://github.com/AspeedTech-BMC/cptra_imgtool;protocol=https;branch=${BRANCH};"

SRC_URI += " file://Cargo.lock "

# Tag for v00.01.04
SRCREV = "27d312fb99cc8f1eab7a89ed866c9595dd6e0e9b"

PV = "1.0+git"

DEPENDS += "caliptra-sw caliptra-mcu-sw"
RDEPENDS:${PN} += "caliptra-sw caliptra-mcu-sw"

inherit cargo cargo-update-recipe-crates

require ${BPN}-crates.inc
require ${BPN}-git-crates.inc

# crates.io may reject the API download endpoint used by BitBake's crate
# fetcher; prefer the static crate tarball location first.
PREMIRRORS:prepend = " \
    https://crates.io/api/v1/crates/([^/]*)/[^/]*/download https://static.crates.io/crates/\1/ \n \
"

do_configure:prepend() {
    install -m 0644 ${UNPACKDIR}/Cargo.lock ${S}/Cargo.lock
}

do_compile() {
    cd ${S}
    # Build cptra_imgtool
    cargo build --offline -p cptra-imgtool --release
    cd -
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${B}/target/release/cptra-imgtool ${D}${bindir}
}

BBCLASSEXTEND = "native nativesdk"

