SUMMARY = "Caliptra MCU firmware and software"
HOMEPAGE = "https://github.com/chipsalliance/caliptra-mcu-sw"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

BRANCH = "main"
SRC_URI = "gitsm://github.com/chipsalliance/caliptra-mcu-sw;protocol=https;branch=${BRANCH};"

SRCREV = "2b7837402328ab611968d40243075082469df7ae"

PV = "1.0+git"

inherit cargo

# Using cargo to download packages
CARGO_DISABLE_BITBAKE_VENDORING = "1"

# Enable network for the compile task allowing cargo to download dependencies
do_compile[network] = "1"

do_compile() {
    cd ${S}

    # Build xtask
    cargo build -p xtask --release
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/target/release/xtask ${D}${bindir}
}

BBCLASSEXTEND = "native nativesdk"
