#@TYPE: Machine
#@NAME: aspeed ast1250
#@DESCRIPTION: Machine configuration for aspeed ast1250 SoC

# Ship all kernel modules by default
MACHINE_EXTRA_RRECOMMENDS = "kernel-modules"

# Allow for MMC booting (required by the NAND-less Beagleboard XM)
EXTRA_IMAGEDEPENDS += "u-boot"

# Uncomment the following line to enable the hard floating point abi. Note that
# this breaks some binary libraries and 3D (neither of which ship with
# meta-yocto). For maximum compatibility, leave this disabled.
require conf/machine/include/tune-arm926ejs.inc

PREFERRED_PROVIDER_virtual/kernel ?= "linux-aspeed"
PREFERRED_VERSION_linux-aspeed ?= "4.1.51"

PREFERRED_VERSION_u-boot ?= "v2016.07"
PREFERRED_VERSION_u-boot-fw-utils ?= "v2016.07"

KERNEL_IMAGETYPE ?= "uImage"
KERNEL_EXTRA_ARGS ?= "UIMAGE_LOADADDR=0x40008000"

UBOOT_SUFFIX ?= "bin"
UBOOT_ENTRYPOINT ?= "0x40008000"
UBOOT_LOADADDRESS ?= "0x40008000"
UBOOT_MACHINE ?= "ast1250_config"

MACHINE_FEATURES = "usbgadget usbhost vfat ext2 serial"
