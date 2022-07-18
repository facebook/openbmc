#
# Copyright 2021-present Facebook. All Rights Reserved.
#
SUMMARY = "cisco-fboss dependencies"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

SRC_URI = " \
    https://raw.githubusercontent.com/facebook/fboss/51fc7ec45ee0357b91217880ae3d65fe4e6a4852/fboss/platform/weutil/WeutilInterface.h;md5sum=4fd6dd8054bd615fb0ceb1dfb0862629 \
    https://raw.githubusercontent.com/facebook/fboss/51fc7ec45ee0357b91217880ae3d65fe4e6a4852/fboss/platform/fw_util/FirmwareUpgradeInterface.h;md5sum=d9c1525b690b4ad5c82437e9e7154a6e \
    https://raw.githubusercontent.com/facebook/fboss/51fc7ec45ee0357b91217880ae3d65fe4e6a4852/fboss/platform/fw_util/firmware_helpers/FirmwareUpgradeHelper.h;md5sum=9c3d73d0d9666ac147da4e8a957a469c \
"

FBOSS_DIR = "${includedir}/fboss"

SYSROOT_DIRS += "${FBOSS_DIR}"

FILES:${PN}-dev := "${FBOSS_DIR}"

do_install() {

    install -d ${D}${FBOSS_DIR}
    install -m 755 -t ${D}${FBOSS_DIR} ${WORKDIR}/*.h

}

