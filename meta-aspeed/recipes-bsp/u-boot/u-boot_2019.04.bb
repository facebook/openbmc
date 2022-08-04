require recipes-bsp/u-boot/u-boot.inc
require u-boot-common.inc

DEPENDS += "bc-native dtc-native"

SRCREV = "dce130c9826afe8b9c445b30784d0da94c37493b"

PV = "v2019.04"
OVERRIDES += ":bld-uboot"

SRCBRANCH = "openbmc/helium/v2019.04"
