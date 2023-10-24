FILESEXTRAPATHS:prepend := "${THISDIR}/6.5:"

LINUX_ASPEED_PATCHES_INC ?= ""
LINUX_ASPEED_PATCHES_INC:openbmc-fb-lf = "linux-patches-6.5.inc"

include ${LINUX_ASPEED_PATCHES_INC}
