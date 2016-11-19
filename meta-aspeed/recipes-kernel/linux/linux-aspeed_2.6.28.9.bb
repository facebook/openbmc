require linux-aspeed.inc

# This revision corresponds to the tag "openbmc/fido/2.6.28"
# We use the revision in order to avoid having to fetch it from the repo during parse
SRCBRANCH = "openbmc/fido/2.6.28"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/theopolis/linux.git;branch=${SRCBRANCH};protocol=https \
           file://patch-2.6.28.9/0001-MTD-fix-m25p80-64-bit-divisions.patch \
           file://patch-2.6.28.9/0005-mtd-Bug-in-m25p80.c-during-whole-chip-erase.patch \
           file://patch-2.6.28.9/0006-mtd-fix-timeout-in-M25P80-driver.patch \
           file://patch-2.6.28.9/0009-mtd-m25p80-timeout-too-short-for-worst-case-m25p16-d.patch \
           file://patch-2.6.28.9/0010-mtd-m25p80-fix-null-pointer-dereference-bug.patch \
           file://patch-2.6.28.9/0012-mtd-m25p80-add-support-for-AAI-programming-with-SST-.patch \
           file://patch-2.6.28.9/0020-mtd-m25p80-make-command-buffer-DMA-safe.patch \
           file://patch-2.6.28.9/0021-mtd-m25p80-Add-support-for-CAT25xxx-serial-EEPROMs.patch \
           file://patch-2.6.28.9/0022-mtd-m25p80-Add-support-for-Macronix-MX25L25635E.patch \
           file://patch-2.6.28.9/0023-mtd-m25p80-Add-support-for-Macronix-MX25L25655E.patch \
           file://patch-2.6.28.9/0024-mtd-m25p80-add-support-for-Micron-N25Q256A.patch \
           file://patch-2.6.28.9/0025-mtd-m25p80-add-support-for-Micron-N25Q128.patch \
           file://patch-2.6.28.9/0026-mtd-m25p80-modify-info-for-Micron-N25Q128.patch \
           file://patch-2.6.28.9/0027-mtd-m25p80-n25q064-is-Micron-not-Intel-Numonyx.patch \
           file://patch-2.6.28.9/0028-ipv6-Plug-sk_buff-leak-in-ipv6_rcv-net-ipv6-ip6_inpu.patch \
           file://patch-2.6.28.9/0029-mtd-m25p80-Add-support-for-the-Winbond-W25Q64.patch \
           file://patch-2.6.28.9/0030-net-Don-t-leak-packets-when-a-netns-is-going-down.patch \
           file://patch-2.6.28.9/0031-IPv6-Print-error-value-when-skb-allocation-fails.patch \
           file://patch-2.6.28.9/0001-bzip2-lzma-library-support-for-gzip-bzip2-and-lzma-d.patch \
           file://patch-2.6.28.9/0002-bzip2-lzma-config-and-initramfs-support-for-bzip2-lz.patch \
          "

S = "${WORKDIR}/git"

LINUX_VERSION ?= "2.6.28.9"
LINUX_VERSION_EXTENSION ?= "-aspeed"

PR = "r2"
PV = "${LINUX_VERSION}"

# Install bounds.h for external module install
# The default install script handles this. However, it looks for bounds.h from
# 'include/generated', which doesnot match 2.6.28, where the file is in
# 'include/linux'.
addtask create_generated after do_compile before do_shared_workdir
do_create_generated() {
    install -d ${B}/include/generated
    cp -l ${B}/include/linux/bounds.h ${B}/include/generated/bounds.h
}

# With Fido, ${KERNEL_SRC} is set to ${STAGING_KERENL_DIR}, which is passed
# to kernel module build. So, copy all .h files from the build direcory to
# the ${STAGING_KERNEL_DIR}
addtask copy_to_kernelsrc after do_shared_workdir before do_compile_kernelmodules
do_copy_to_kernelsrc() {
    kerneldir=${STAGING_KERNEL_DIR}/include/linux
    install -d ${kerneldir}
    cp -l ${B}/include/linux/* ${kerneldir}/
}

KERNEL_CC += " --sysroot=${PKG_CONFIG_SYSROOT_DIR}"
