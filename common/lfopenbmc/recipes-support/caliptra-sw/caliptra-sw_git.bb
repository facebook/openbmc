SUMMARY = "Caliptra firmware and software"
HOMEPAGE = "https://github.com/chipsalliance/caliptra-sw"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit cargo cargo-update-recipe-crates

BRANCH = "aspeed-rt-1.2.0"
SRC_URI = "gitsm://github.com/AspeedTech-BMC/caliptra-sw;protocol=https;branch=${BRANCH} \
           file://0001-Cargo.lock-add-missing-thiserror-2.0.18-entries.patch \
           file://0002-Cargo.lock-resolve-time-0.3.44-deranged-deps.patch \
           "

# Tag for v01.02.05
SRCREV = "4a2fd14773e8ddcc7750346c47be7638a5dd4da2"

PV = "1.0+git"

# The ufmt-macros git submodule dependency uses #![deny(warnings)]. Newer Rust
# toolchains promote the mismatched_lifetime_syntaxes lint, turning those
# warnings into hard errors. Cargo only auto-caps lints for registry crates, not
# for this path/submodule crate, so cap all lints at warn to keep the build green.
export RUSTFLAGS = "--cap-lints warn"

CARGO_SRC_DIR = "auth-manifest/app/"

# caliptra-sw is a Cargo workspace; the Cargo.lock lives at the repo root,
# not in the auth-manifest/app member dir that CARGO_SRC_DIR points at.
CARGO_LOCK_PATH = "${S}/Cargo.lock"

require ${BPN}-crates.inc
require ${BPN}-git-crates.inc

# crates.io may reject the API download endpoint used by BitBake's crate
# fetcher; prefer the static crate tarball location first.
PREMIRRORS:prepend = " \
    https://crates.io/api/v1/crates/([^/]*)/[^/]*/download https://static.crates.io/crates/\1/ \n \
"

do_compile() {
    cd ${S}
    # Build caliptra-auth-manifest-app
    cargo build --offline --release --manifest-path=auth-manifest/app/Cargo.toml
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/target/release/caliptra-auth-manifest-app ${D}${bindir}
}

BBCLASSEXTEND = "native nativesdk"
