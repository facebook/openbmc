inherit cargo

BBCLASSEXTEND = "native"

# Make sure we only depend on rust-native, not on the cross compilation
# toolchain.
DEPENDS = ""
DEPENDS:class-native = "rust-native qemu-system-native "
RDEPENDS:${PN}:class-native:append = " qemu-system-native"

oe_cargo_build:prepend = "export STAGING_BINDIR_NATIVE=${STAGING_BINDIR_NATIVE}"

# Generated with "cargo bitbake".
SRC_URI = " \
    crate://crates.io/anyhow/1.0.61 \
    crate://crates.io/atty/0.2.14 \
    crate://crates.io/autocfg/1.1.0 \
    crate://crates.io/bitflags/1.3.2 \
    crate://crates.io/cfg-if/1.0.0 \
    crate://crates.io/clap/3.2.16 \
    crate://crates.io/clap_derive/3.2.15 \
    crate://crates.io/clap_lex/0.2.4 \
    crate://crates.io/fastrand/1.8.0 \
    crate://crates.io/fdt/0.1.4 \
    crate://crates.io/hashbrown/0.12.3 \
    crate://crates.io/heck/0.4.0 \
    crate://crates.io/hermit-abi/0.1.19 \
    crate://crates.io/indexmap/1.9.1 \
    crate://crates.io/instant/0.1.12 \
    crate://crates.io/libc/0.2.131 \
    crate://crates.io/once_cell/1.13.0 \
    crate://crates.io/os_str_bytes/6.2.0 \
    crate://crates.io/proc-macro-error-attr/1.0.4 \
    crate://crates.io/proc-macro-error/1.0.4 \
    crate://crates.io/proc-macro2/1.0.43 \
    crate://crates.io/quote/1.0.21 \
    crate://crates.io/redox_syscall/0.2.16 \
    crate://crates.io/remove_dir_all/0.5.3 \
    crate://crates.io/strsim/0.10.0 \
    crate://crates.io/syn/1.0.99 \
    crate://crates.io/tempfile/3.3.0 \
    crate://crates.io/termcolor/1.1.3 \
    crate://crates.io/textwrap/0.15.0 \
    crate://crates.io/unicode-ident/1.0.3 \
    crate://crates.io/version_check/0.9.4 \
    crate://crates.io/winapi-i686-pc-windows-gnu/0.4.0 \
    crate://crates.io/winapi-util/0.1.5 \
    crate://crates.io/winapi-x86_64-pc-windows-gnu/0.4.0 \
    crate://crates.io/winapi/0.3.9 \
"

LOCAL_URI = " \
    file://Cargo.toml \
    file://Cargo.lock \
    file://src \
    file://LICENSE \
"
LIC_FILES_CHKSUM = " \
    file://LICENSE;md5=4cc91856b08b094b4f406a29dc61db21 \
    file://src/main.rs;beginline=1;endline=1;md5=fcab174c20ea2e2bc0be64b493708266 \
"

SUMMARY = "fb wrapper for QEMU"
HOMEPAGE = "https://github.com/facebook/openbmc"
LICENSE = "GPL-2.0-only"

addtask addto_recipe_sysroot after do_populate_sysroot before do_build
