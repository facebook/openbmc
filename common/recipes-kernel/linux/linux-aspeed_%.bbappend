FILESEXTRAPATHS:prepend := "${THISDIR}/6.1:"

LINUX_ASPEED_PATCHES_INC ?= ""
LINUX_ASPEED_PATCHES_INC:openbmc-fb-lf = "linux-patches-6.1.inc"

include ${LINUX_ASPEED_PATCHES_INC}
