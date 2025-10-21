LINUX_ASPEED_PATCHES_INC ?= ""
LINUX_ASPEED_PATCHES_INC:openbmc-fb-lf = "linux-patches-6.6.inc"

KBRANCH:openbmc-fb-lf ?= "dev-6.6"
LINUX_VERSION:openbmc-fb-lf ?= "6.6.105"

SRCREV:openbmc-fb-lf = "82a00f69c382193027bb1cf31acaf964b76a0eaa"

include ${LINUX_ASPEED_PATCHES_INC}
