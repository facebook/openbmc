require recipes-bsp/u-boot/u-boot.inc
require u-boot-common.inc

DEPENDS += "bc-native dtc-native"

OVERRIDES:append = ":bld-uboot"

SRCBRANCH = "openbmc/helium/v2019.04"

PV = "v2019.04+git${SRCPV}"
include common/recipes-bsp/u-boot-fbobmc/use-intree-shipit.inc
