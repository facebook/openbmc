LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://zephyr/LICENSE;md5=fa818a259cbed7ce8bc2a22d35a464fc"

CVE_PRODUCT = "zephyr"

inherit cmake

# This file might be included from other places (like other layers) and not
# having an explicit path to the patches directory, will make bitbake fail to
# find the patch(es) in SRC_URI.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

require recipes-kernel/zephyr-kernel/zephyr-kernel-src-${PREFERRED_VERSION_zephyr-kernel}.inc
