BPN = "qemu"

require recipes-devtools/qemu/qemu-native.inc

DEPENDS = "glib-2.0-native \
           zlib-native \
           pixman-native \
           qemu-native \
           bison-native \
           ninja-native \
          "

S = "${WORKDIR}/git"

SRCREV_FORMAT = "dtc_meson_slirp_berkeley-softfloat-3_berkeley-testfloat-3_keycodemapdb"
SRCREV_dtc = "b6910bec11614980a21e46fbccc35934b671bd81"
SRCREV_meson = "12f9f04ba0decfda425dbbf9a501084c153a2d18"
SRCREV_slirp = "9d59bb775d6294c8b447a88512f7bb43f12a25a8"
SRCREV_berkeley-softfloat-3 = "b64af41c3276f97f0e181920400ee056b9c88037"
SRCREV_berkeley-testfloat-3 = "5a59dcec19327396a011a17fd924aed4fec416b3"
SRCREV_keycodemapdb = "d21009b1c9f94b740ea66be8e48a1d8ad8124023"
SRCREV = "430a388ef4a6e02e762a9c5f86c539f886a6a61a"

SRC_URI = "git://gitlab.com/qemu-project/qemu.git;branch=master;protocol=https \
           git://gitlab.com/qemu-project/dtc.git;destsuffix=git/dtc;protocol=https;name=dtc;nobranch=1 \
           git://gitlab.com/qemu-project/meson.git;destsuffix=git/meson;nobranch=1;protocol=https;name=meson;nobranch=1 \
           git://gitlab.com/qemu-project/libslirp.git;destsuffix=git/slirp;nobranch=1;protocol=https;name=slirp;nobranch=1 \
           git://gitlab.com/qemu-project/berkeley-softfloat-3.git;destsuffix=git/tests/fp/berkeley-softfloat-3;nobranch=1;protocol=https;name=berkeley-softfloat-3;nobranch=1 \
           git://gitlab.com/qemu-project/berkeley-testfloat-3.git;destsuffix=git/tests/fp/berkeley-testfloat-3;nobranch=1;protocol=https;name=berkeley-testfloat-3;nobranch=1 \
           git://gitlab.com/qemu-project/keycodemapdb.git;destsuffix=git/ui/keycodemapdb;nobranch=1;protocol=https;name=keycodemapdb;nobranch=1 \
           file://0001-aspeed-Zero-extend-flash-files-to-128MB.patch \
           "
PV = "7.0.90+git${SRCPV}"

PACKAGECONFIG ??= "fdt "

EXTRA_OECONF:append = " --target-list=${@get_qemu_system_target_list(d)}"
EXTRA_OECONF:remove = " --meson=meson"

do_install:append() {
    # The following is also installed by qemu-native
    rm -f ${D}${datadir}/qemu/trace-events-all
    rm -rf ${D}${datadir}/qemu/keymaps
    rm -rf ${D}${datadir}/icons/
    rm -rf ${D}${includedir}/qemu-plugin.h
}
