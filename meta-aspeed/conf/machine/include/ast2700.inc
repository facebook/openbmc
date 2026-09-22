# Copyright 2026-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

# Backported from https://github.com/openbmc/openbmc/commit/1ceb15e419a02077532407554e77a7368448d750

SOC_MODEL ?= "ast2700"
SOC_FAMILY = "aspeed-g7"
include conf/machine/include/soc-family.inc
require conf/machine/include/aspeed.inc

DEFAULTTUNE = "cortexa35"

require conf/machine/include/arm/armv8a/tune-cortexa35.inc
require conf/machine/include/bootmcu.inc

UBOOT_FIT_ARM_TRUSTED_FIRMWARE = "1"
UBOOT_FIT_TEE = "1"

SERIAL_CONSOLES ?= "115200;ttyS12"

# AST2700 is only supported on kernel 6.18+; mask older recipes
# and their bbappends to avoid missing defconfig/dangling appends
BBMASK:append = " meta-aspeed/recipes-kernel/linux/linux-aspeed_4.1"
BBMASK:append = " meta-aspeed/recipes-kernel/linux/linux-aspeed_[5]"
BBMASK:append = " meta-aspeed/recipes-kernel/linux/linux-aspeed_6.0.bb"
BBMASK:append = " meta-aspeed/recipes-kernel/linux/linux-aspeed_6.1.bb"
BBMASK:append = " meta-aspeed/recipes-kernel/linux/linux-aspeed_6.6.bb"
BBMASK:append = " meta-aspeed/recipes-kernel/linux/linux-aspeed_6.12.bb"

# For runqemu
QB_SYSTEM_NAME = "qemu-system-aarch64"
QB_MACHINE = "-machine ast2700a1-evb"
QB_MEM = "-m 1G"

# bootloader fitimage
UBOOT_SUFFIX ?= "bin"
UBOOT_FIT_UBOOT_ENTRYPOINT ?= "0x80000000"
UBOOT_FIT_UBOOT_LOADADDRESS ?= "0x80000000"
UBOOT_FIT_ARM_TRUSTED_FIRMWARE_LOADADDRESS ?= "0xb0000000"
UBOOT_FIT_ARM_TRUSTED_FIRMWARE_ENTRYPOINT ?= "0xb0000000"
UBOOT_FIT_TEE_LOADADDRESS ?= "0xb0080000"
UBOOT_FIT_TEE_ENTRYPOINT ?= "0xb0080000"

# kernel fitimage
# Use lzma compressed algorithm instead of gzip to save the code size.
FIT_KERNEL_COMP_ALG ?= "lzma"
FIT_KERNEL_COMP_ALG_EXTENSION ?= ".lzma"

FIT_ADDRESS_CELLS ?= "2"
UBOOT_ENTRYPOINT ?= "0x4 0x00000000"
UBOOT_LOADADDRESS ?= "0x4 0x00000000"

COMPATIBLE_MACHINE:aspeed-g7 = "aspeed-g7"
TFA_PLATFORM = "ast2700"

# optee for BL32.
TFA_SPD = "opteed"

# TFA default build is BL31.
TFA_BUILD_TARGET = "bl31"

# It is a shortcut for "make PLATFORM=aspeed PLATFORM_FLAVOR=ast2700"
OPTEEMACHINE = "aspeed-ast2700"

UBOOT_FIT_ARM_TRUSTED_FIRMWARE_IMAGE ?= "${DEPLOY_DIR_IMAGE}/${TFA_BUILD_TARGET}.bin"
UBOOT_FIT_TEE_IMAGE ?= "${DEPLOY_DIR_IMAGE}/optee/tee-raw.bin"

# bootmcu
PREFERRED_PROVIDER_virtual/bootmcu ?= "bootmcu-spl"
