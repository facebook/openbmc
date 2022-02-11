#@TYPE: Machine
#@NAME: Aspeed AST2620
#@DESCRIPTION: Machine configuration for Aspeed AST2620 SoC

SOC_MODEL ?= "ast2620"
SOC_FAMILY = "aspeed-g6"

include conf/machine/include/soc-family.inc
include conf/machine/include/arm/arch-armv7a.inc

# Ship all kernel modules by default
MACHINE_EXTRA_RRECOMMENDS = "kernel-modules"

EXTRA_IMAGEDEPENDS += "u-boot"

DEFAULTTUNE ?= "armv7a-vfpv4d16"
PREFERRED_PROVIDER_virtual/kernel ?= "linux-aspeed"

PREFERRED_VERSION_linux-aspeed ?= "5.3.8"

PREFERRED_VERSION_u-boot ?= "v2019.04"
PREFERRED_VERSION_u-boot-tools ?= "v2019.04"
PREFERRED_VERSION_u-boot-fw-utils ?= "v2019.04"

KERNEL_IMAGETYPE ?= "zImage"
KERNEL_EXTRA_ARGS ?= "UIMAGE_LOADADDR=0x80008000"

UBOOT_SUFFIX ?= "bin"
UBOOT_ENTRYPOINT ?= "0x80008000"
UBOOT_LOADADDRESS ?= "0x80008000"
UBOOT_MACHINE ?= "ast-g6_config"
