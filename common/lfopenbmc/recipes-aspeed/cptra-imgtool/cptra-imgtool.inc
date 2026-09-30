SUMMARY = "Generate CPTRA authorization flash image with cptra_imgtool"
HOMEPAGE = "https://github.com/AspeedTech-BMC/cptra_imgtool"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

BRANCH = "master"
SRC_URI = "git://github.com/AspeedTech-BMC/cptra_imgtool;protocol=https;branch=${BRANCH} \
           file://0001-ufmt-macros-fix-lifetimes-for-rust-1.94.patch;patchdir=${UNPACKDIR}/ufmt \
           "

# Tag for v00.01.05
SRCREV = "3c06158958aeac6b8aa537a3da67331e730e4562"

DEPENDS += "caliptra-mcu-sw"

inherit cargo cargo-update-recipe-crates

# crates.io may reject the API download endpoint used by BitBake's crate
# fetcher; prefer the static crate tarball location first.
PREMIRRORS:prepend = " \
    https://crates.io/api/v1/crates/([^/]*)/[^/]*/download https://static.crates.io/crates/\1/ \n \
"

do_compile() {
    cd ${S}
    # Build caliptra-auth-manifest-app
    cargo build --offline -p ${CALIPTRA_AUTH_MANIFEST_APP} --release

    # Build cptra_imgtool
    cargo build --offline -p cptra-imgtool --release
    cd -
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${B}/target/release/${CALIPTRA_AUTH_MANIFEST_APP} ${D}${bindir}
    install -m 0755 ${B}/target/release/cptra-imgtool ${D}${bindir}
}

BBCLASSEXTEND = "native nativesdk"
