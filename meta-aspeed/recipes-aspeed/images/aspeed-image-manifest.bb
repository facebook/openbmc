# Copyright 2026-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

DESCRIPTION = "Generate ASPEED Caliptra Manifest image"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"
PACKAGE_ARCH = "${MACHINE_ARCH}"

S = "${UNPACKDIR}"

SRC_URI = "file://${CALIPTRA_MANIFEST_CONFIG}"

PR = "r0"

do_patch[noexec] = "1"
do_configure[noexec] = "1"
do_install[noexec] = "1"

inherit deploy

DEPENDS += "cptra-imgtool-native"

CALIPTRA_MANIFEST_FLASH_IMAGE ?= "ast2700-manifest-flash.bin"
CALIPTRA_MANIFEST_SOC_IMAGE ?= "ast2700-soc-manifest.bin"
CALIPTRA_MANIFEST_CONFIG ?= "ast2700-default-manifest.toml"

# Using cptra-imgtool to create manifest image.
create_cptra_manifest_image() {
    export RUST_LOG="debug"

    local caliptra_manifest_key_dir=""

    if [ -n "${CALIPTRA_MANIFEST_KEY_DIR}" ]; then
        caliptra_manifest_key_dir="--key-dir ${CALIPTRA_MANIFEST_KEY_DIR}/"
    fi

    echo "caliptra_manifest_key_dir=${caliptra_manifest_key_dir}"

    # Build the Caliptra Flash Image (including the Caliptra SoC manifest).
    cptra-imgtool \
        create-auth-flash \
        --cfg ${UNPACKDIR}/${CALIPTRA_MANIFEST_CONFIG} \
        ${caliptra_manifest_key_dir} \
        --prebuilt-dir ${DEPLOY_DIR_IMAGE}/ \
        --flash ${B}/${CALIPTRA_MANIFEST_FLASH_IMAGE}

    # Build only the Caliptra SoC Manifest.
    cptra-imgtool \
        create-auth-man \
        --cfg ${UNPACKDIR}/${CALIPTRA_MANIFEST_CONFIG} \
        ${caliptra_manifest_key_dir} \
        --prebuilt-dir ${DEPLOY_DIR_IMAGE}/ \
        --man ${B}/${CALIPTRA_MANIFEST_SOC_IMAGE}
}

do_compile() {
    create_cptra_manifest_image
}

do_compile[depends] += " \
    optee-os:do_deploy \
    trusted-firmware-a:do_deploy \
    virtual/bootloader:do_deploy \
    virtual/bootmcu:do_deploy \
    bmc-pb:do_deploy \
    ${@bb.utils.contains('MACHINE_FEATURES', 'ast-ssp', 'virtual/ssp:do_deploy', '', d)} \
    ${@bb.utils.contains('MACHINE_FEATURES', 'ast-tsp', 'virtual/tsp:do_deploy', '', d)} \
    "

do_deploy() {
    install -d ${DEPLOYDIR}
    install -m 644 ${B}/${CALIPTRA_MANIFEST_FLASH_IMAGE} ${DEPLOYDIR}
    install -m 644 ${B}/${CALIPTRA_MANIFEST_SOC_IMAGE} ${DEPLOYDIR}
}

addtask deploy before do_build after do_compile
