#@TYPE: Machine
#@NAME: FBOSS-Lite
#@DESCRIPTION: Machine configuration for Facebook FBOSS-Lite

require conf/machine/include/ast2620.inc
require conf/machine/include/fb-fboss.inc
require conf/machine/include/mtd-ubifs.inc
require conf/machine/include/tpm2.inc

KERNEL_IMAGETYPE = "zImage"
UBOOT_CONFIG_BASE = "fbobmc-ast-g6_defconfig"

PREFERRED_VERSION_u-boot = "v2019.04%"
PREFERRED_VERSION_u-boot-tools = "v2019.04%"
PREFERRED_VERSION_u-boot-tools-native = "v2019.04%"
PREFERRED_VERSION_nativesdk-u-boot-tools = "v2019.04%"
PREFERRED_VERSION_u-boot-fw-utils = "v2019.04%"
PREFERRED_VERSION_u-boot-fw-utils-cross = "v2019.04%"

# Select wedge-eeprom-0.2 to support Meta EEPROM format v4.
PREFERRED_VERSION_wedge-eeprom = "0.2"
PREFERRED_VERSION_libwedge-eeprom = "0.2"

MACHINE_FEATURES += "emmc emmc-ext4"

MACHINE_ESSENTIAL_EXTRA_RDEPENDS += "kernel-module-i2c-dev-sysfs"

EXTRA_IMAGE_FEATURES:append = " image-meta"
FW_ENV_CONFIG_FILE = "fw_env.config.64k"
