#@TYPE: Machine
#@NAME: Aspeed AST2520
#@DESCRIPTION: Machine configuration for Aspeed AST2520 SoC

# Ship all kernel modules by default
MACHINE_EXTRA_RRECOMMENDS = "kernel-modules"

EXTRA_IMAGEDEPENDS += "u-boot"

# Uncomment the following line to enable the hard floating point abi. Note that
# this breaks some binary libraries and 3D (neither of which ship with
# meta-yocto). For maximum compatibility, leave this disabled.
DEFAULTTUNE ?= "armv6-novfp"
require conf/machine/include/tune-arm1136jf-s.inc

PREFERRED_PROVIDER_virtual/kernel ?= "linux-aspeed"

# We have 2 kernel trees to pick from
PREFERRED_VERSION_linux-aspeed ?= "4.1.51"
#PREFERRED_VERSION_linux-aspeed ?= "4.7"

PREFERRED_VERSION_u-boot ?= "v2016.07"
PREFERRED_VERSION_u-boot-fw-utils ?= "v2016.07"

KERNEL_IMAGETYPE ?= "uImage"
KERNEL_EXTRA_ARGS ?= "UIMAGE_LOADADDR=0x80008000"

UBOOT_SUFFIX ?= "bin"
UBOOT_ENTRYPOINT ?= "0x80008000"
UBOOT_LOADADDRESS ?= "0x80008000"
UBOOT_MACHINE ?= "ast-g5_config"

MACHINE_FEATURES = "usbgadget usbhost vfat ext2 serial"
