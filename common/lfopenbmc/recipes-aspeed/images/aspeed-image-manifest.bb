DESCRIPTION = "Generate ASPEED Caliptra Manifest image"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"
PACKAGE_ARCH = "${MACHINE_ARCH}"

S = "${UNPACKDIR}"

SRC_URI = " \
    file://configs \
    file://keys \
    "

PR = "r0"

do_patch[noexec] = "1"
do_configure[noexec] = "1"
do_install[noexec] = "1"

inherit deploy

DEPENDS += "cptra-imgtool-native"

CALIPTRA_MANIFEST_FLASH_IMAGE ?= "ast2700-manifest-flash.bin"
CALIPTRA_MANIFEST_SOC_IMAGE ?= "ast2700-soc-manifest.bin"
CALIPTRA_MANIFEST_CONFIG ?= "ast2700-default-ecc-manifest.toml"
CALIPTRA_MANIFEST_CONFIG_DIR ?= "${UNPACKDIR}/configs"
CALIPTRA_MANIFEST_KEY_DIR ?= "${UNPACKDIR}/keys"

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
        --cfg ${CALIPTRA_MANIFEST_CONFIG_DIR}/${CALIPTRA_MANIFEST_CONFIG} \
        ${caliptra_manifest_key_dir} \
        --prebuilt-dir ${DEPLOY_DIR_IMAGE}/ \
        --flash ${B}/${CALIPTRA_MANIFEST_FLASH_IMAGE}

    # Build only the Caliptra SoC Manifest.
    cptra-imgtool \
        create-auth-man \
        --cfg ${CALIPTRA_MANIFEST_CONFIG_DIR}/${CALIPTRA_MANIFEST_CONFIG} \
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
    "

do_deploy() {
    install -d ${DEPLOYDIR}
    install -m 644 ${B}/${CALIPTRA_MANIFEST_FLASH_IMAGE} ${DEPLOYDIR}
    install -m 644 ${B}/${CALIPTRA_MANIFEST_SOC_IMAGE} ${DEPLOYDIR}
}

addtask deploy before do_build after do_compile
