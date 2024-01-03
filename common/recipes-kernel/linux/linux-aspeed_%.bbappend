FILESEXTRAPATHS:prepend := "${THISDIR}/6.6:"

LINUX_ASPEED_PATCHES_INC ?= ""
LINUX_ASPEED_PATCHES_INC:openbmc-fb-lf = "linux-patches-6.6.inc"

include ${LINUX_ASPEED_PATCHES_INC}
