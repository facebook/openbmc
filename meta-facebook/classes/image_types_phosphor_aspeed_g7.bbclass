# According to the design of AST2700, bootmcu(riscv-32) execute virtual/bootmcu and CPU(coretax-a35) execute u-boot.
# We added the do_merge_uboot task to generate u-boot image before do_generate_static
# to ensure compatibility with image_types_phosphor.bbclass.
UBOOT_BINARY := "${CALIPTRA_MANIFEST_FLASH_IMAGE}"
UBOOT_SUFFIX:append = ".merged"

# Install the image-u-boot to deploy folder when building the emmc image.
do_generate_ext4_tar:append() {
    cd ${S}/ext4
    install -m 644 image-u-boot ${IMGDEPLOYDIR}/image-u-boot
}

do_merge_uboot() {
    # Starting from AST2700 A2, the Caliptra Manifest Flash image includes the following components:
    # Caliptra Firmware Pre-built Image
    # MCU Runtime Binary
    # Caliptra SoC Manifest
    # ARM Trusted Firmware (ATF)
    # OPTEE OS
    # U-Boot Raw Image
    # DDR4/DDR5 Pre-built Images
    # SSP Firmware Binary and TSP Firmware Binary (optional, depending on user requirements)
    #
    # Therefore, u-boot.bin.merged, image-u-boot, and the Caliptra Manifest Flash image
    # are identical for AST2700 A2 and later.

    install -m 644 ${DEPLOY_DIR_IMAGE}/${UBOOT_BINARY} ${DEPLOY_DIR_IMAGE}/u-boot.${UBOOT_SUFFIX}
}

# Using aspeed-image-manifest to generate the SoC manifest image.
do_merge_uboot[depends] += " \
    u-boot:do_deploy \
    aspeed-image-manifest:do_deploy \
    "

addtask do_merge_uboot before do_generate_static after do_generate_rwfs_static

do_make_ubi[depends] += "${PN}:do_merge_uboot"
do_generate_ubi_tar[depends] += "${PN}:do_merge_uboot"
do_generate_static_tar[depends] += "${PN}:do_merge_uboot"
do_generate_static_norootfs[depends] += "${PN}:do_merge_uboot"
do_generate_ext4_tar[depends] += "${PN}:do_merge_uboot"
