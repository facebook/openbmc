# Copyright 2022-present Facebook. All Rights Reserved.

SUMMARY = "Dummy ROM used by qemu"
DESCRIPTION = "A dummy rom code used by QEMU locate & load uboot from the image"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRC_URI = "git://github.com/google/vbootrom;protocol=https;branch=master${@local_files_to_sourcedir(d)} \
           file://0001-Add-log-message-for-copying-boot-image.patch \
           file://0002-Load-npcm8xx-u-boot-dynamically.patch \
           "

# Modify these as desired
PV = "1.0+git${SRCPV}"
SRCREV = "1287b6e42e839ba2ab0f06268c5b53ae60df3537"

S = "${WORKDIR}/git"
B = "${S}/npcm8xx"

export CROSS_COMPILE="${TARGET_PREFIX}"

inherit deploy

do_deploy () {
	install -D -m 644 ${B}/npcm8xx_bootrom.bin ${DEPLOYDIR}/qemu_bios.bin
}

addtask deploy before do_build after do_compile
