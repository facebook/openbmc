SRC_URI_remove = " \
            file://0012-fix-libcap-header-issue-on-some-distro.patch \
            file://0013-cpus.c-Add-error-messages-when-qemi_cpu_kick_thread-.patch \
            "

EXTRA_OECONF_remove = "--with-confsuffix=/${BPN}"
EXTRA_OECONF_remove = "--disable-git-update"
EXTRA_OECONF_append = " --with-git-submodules=ignore"

PACKAGECONFIG[bluez] = ""
PACKAGECONFIG[libxml2] = ""
PACKAGECONFIG[xfs] = ""
PACKAGECONFIG[vnc-png] = "--enable-vnc --enable-png,--disable-vnc --disable-png,libpng"

do_install_append() {
    # The following is also installed by qemu-native
    rm -f ${D}${datadir}/qemu/trace-events-all
    rm -rf ${D}${datadir}/qemu/keymaps
    rm -rf ${D}${datadir}/icons/
}
