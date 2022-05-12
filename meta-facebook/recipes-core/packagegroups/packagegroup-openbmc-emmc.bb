SUMMARY = "Facebook OpenBMC eMMC Package Group"

LICENSE = "GPL-2.0-or-later"
PR = "r1"

inherit packagegroup

EMMC_EXT4_REQUIRED_PACKAGES = "\
  e2fsprogs \
"

# Include e2fsprogs here for a short time until we transition the mounting
# scripts to fully use btrfs.
EMMC_BTRFS_REQUIRED_PACKAGES = "\
  btrfs-tools \
  e2fsprogs \
"

EMMC_REQUIRED_PACKAGES = "\
  util-linux \
  mmc-utils \
  blktrace \
  mmc-raw \
  emmcd \
  ${@bb.utils.contains('MACHINE_FEATURES', 'emmc-ext4', \
                       '${EMMC_EXT4_REQUIRED_PACKAGES}', \
                       '${EMMC_BTRFS_REQUIRED_PACKAGES}', d)} \
"

RDEPENDS:${PN} += "\
  ${@bb.utils.contains('MACHINE_FEATURES', 'emmc', '${EMMC_REQUIRED_PACKAGES}', '', d)} \
  "
