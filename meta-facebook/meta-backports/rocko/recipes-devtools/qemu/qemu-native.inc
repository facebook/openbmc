include recipes-devtools/qemu/qemu.inc

LIC_FILES_CHKSUM = "file://COPYING;md5=441c28d2cf86e15a37fa47e15a72fbac \
                    file://COPYING.LIB;endline=24;md5=8c5efda6cf1e1b03dcfd0e6c0d271c7f"

EXTRA_OECONF_remove = "--with-confsuffix=/${BPN}"
EXTRA_OECONF_remove = "--disable-static"

PACKAGECONFIG[bluez] = ""
PACKAGECONFIG[ssh2] = ""
PACKAGECONFIG[xfs] = ""
PACKAGECONFIG[vnc-png] = "--enable-vnc --enable-png,--disable-vnc --disable-png,libpng"

def get_qemu_system_target_list(d):
    return get_qemu_target_list(d)
