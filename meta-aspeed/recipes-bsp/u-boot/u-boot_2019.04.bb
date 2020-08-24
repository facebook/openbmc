require recipes-bsp/u-boot/u-boot.inc
require u-boot-common.inc

DEPENDS += "bc-native dtc-native"

PV = "v2019.04"
OVERRIDES += ":bld-uboot"

SRCBRANCH = "openbmc/helium/v2019.04"
