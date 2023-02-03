SUMMARY = "Linux kernel USBIP tools"
SECTION = "kernel"
LICENSE = "GPL-2.0-or-later"

inherit linux-kernel-base
inherit module-base
inherit autotools pkgconfig

# checkout the kernel code
do_configure[depends] += "virtual/kernel:do_shared_workdir"
do_configure[depends] += "virtual/kernel:do_install"

# skip the tasks
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_patch[noexec] = "1"
deltask do_populate_sysroot

DEPENDS = " udev tcp-wrappers "
RDEPENDS:${PN} = " tcp-wrappers udev hwdata "
S = "${WORKDIR}/src"

# configure the source code
do_configure:prepend () {
  mkdir -p ${S}
  cp -rf ${STAGING_KERNEL_DIR}/tools/usb/usbip/* ${S}
  sed -i 's/-Werror//g' ${S}/configure.ac
}
