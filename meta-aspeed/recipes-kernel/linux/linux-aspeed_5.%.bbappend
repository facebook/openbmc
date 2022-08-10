SRC_URI += "\
    ${@bb.utils.contains('MACHINE_FEATURES', 'mtd-ubifs', \
                         'file://ubifs.scc file://ubifs.cfg', '', d)} \
    ${@bb.utils.contains('MACHINE_FEATURES', 'emmc', \
        bb.utils.contains('MACHINE_FEATURES', 'emmc-ext4', \
                           'file://emmc-ext4.scc file://emmc-ext4.cfg', \
                           'file://emmc-btrfs.scc file://emmc-btrfs.cfg', d), \
        '', d)} \
    "

KERNEL_FEATURES:append = " \
    ${@bb.utils.contains('MACHINE_FEATURES', 'mtd-ubifs', \
                         'ubifs.scc', '', d)} \
    ${@bb.utils.contains('MACHINE_FEATURES', 'emmc', \
        bb.utils.contains('MACHINE_FEATURES', 'emmc-ext4', \
                             'emmc-ext4.scc', 'emmc-btrfs.scc', d), '', d)} \
    "
