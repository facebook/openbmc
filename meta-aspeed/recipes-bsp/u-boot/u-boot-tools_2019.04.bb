require u-boot-common.inc
require u-boot-tools.inc

PV = "v2019.04+git${SRCPV}"
SRCBRANCH = "openbmc/helium/v2019.04"

include common/recipes-bsp/u-boot-fbobmc/use-intree-shipit.inc
