SUMMARY = "Caliptra MCU firmware and software"
HOMEPAGE = "https://github.com/chipsalliance/caliptra-mcu-sw"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

BRANCH = "main"
SRC_URI = "gitsm://github.com/chipsalliance/caliptra-mcu-sw;protocol=https;branch=${BRANCH} \
           file://0001-Cargo.lock-remove-stale-ufmt-0.1.0-entries.patch \
           file://0001-ufmt-macros-fix-lifetimes-for-rust-1.94.patch;patchdir=${UNPACKDIR}/ufmt \
           "

SRCREV = "2b7837402328ab611968d40243075082469df7ae"

PV = "1.0+git"

inherit cargo cargo-update-recipe-crates

require ${BPN}-crates.inc
require ${BPN}-git-crates.inc

# crates.io may reject the API download endpoint used by BitBake's crate
# fetcher; prefer the static crate tarball location first.
PREMIRRORS:prepend = " \
    https://crates.io/api/v1/crates/([^/]*)/[^/]*/download https://static.crates.io/crates/\1/ \n \
"

do_compile() {
    cd ${S}

    # Build xtask
    cargo build --offline -p xtask --release
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/target/release/xtask ${D}${bindir}
}

BBCLASSEXTEND = "native nativesdk"
