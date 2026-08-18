BPN = "qemu-master"

# Include target list functions BEFORE qemu-master-native_git.inc (which has inherit native)
require qemu-master-targets.inc

require qemu-master-native_git.inc

# As some of the files installed by qemu-master-native and qemu-system-native
# are the same, we depend on qemu-master-native to get the full installation set
# and avoid file clashes
DEPENDS += "glib-2.0-native zlib-native pixman-native qemu-master-native python3-qemu-qmp-native"

# subprojects/usbredir also configures usbredirhost, which requires libusb
# even though QEMU only consumes libusbredirparser-0.5.
DEPENDS += "libusb1-native"

EXTRA_OECONF:append = " --target-list=${@get_qemu_system_target_list(d)}"

PACKAGECONFIG ??= "fdt alsa kvm pie slirp png pixman usb-redir \
    ${@bb.utils.contains('DISTRO_FEATURES', 'opengl', 'virglrenderer epoxy', '', d)} \
"

# Handle distros such as CentOS 5 32-bit that do not have kvm support
PACKAGECONFIG:remove = "${@'kvm' if not os.path.exists('/usr/include/linux/kvm.h') else ''}"

do_install:append() {
    install -Dm 0755 ${UNPACKDIR}/powerpc_rom.bin ${D}${datadir}/qemu

    # The following is also installed by qemu-master-native
    rm -f ${D}${datadir}/qemu/trace-events-all
    rm -rf ${D}${datadir}/qemu/keymaps
    rm -rf ${D}${datadir}/icons/
    rm -rf ${D}${includedir}/qemu-plugin.h
}
