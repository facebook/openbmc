IMAGE_FEATURES[validitems] += "image-meta"

inherit ${@bb.utils.contains("IMAGE_FEATURES", "image-meta", \
  "fbobmc-image", "kernel_fitimage", d)}

def fbobmc_get_image_part_table(d):
    # define the partition table of image with (offset, name, max_size)
    # the max_size is used to do sanity check of the partition define
    # it can be omitted as -1, which will be calculated automatically
    # the order in the partition in the list does not matter
    # sanity check will make sure the partitions not overlap, no gaps
    # align with erase_block_size and fits in the space defined as image_max_size
    image_max_size = 32 * 1024 * 1024
    erase_block_size = 64 * 1024
    image_part_table_vboot = (
        #  offset,      name            max_size (K)    type            src_component ( 0: fill with zero, None: skip)
        (  0,           "spl",          256         ,   "rom",      d.expand("${DEPLOY_DIR_IMAGE}/u-boot-spl-${MACHINE}.${UBOOT_SUFFIX}") ),
        (  0x00040000,  "rec-u-boot",   576         ,   "raw",      d.expand("${DEPLOY_DIR_IMAGE}/u-boot-recovery-${MACHINE}.${UBOOT_SUFFIX}")),
        (  0x000D0000,  "u-boot-env",   128         ,   "data",     0),
        (  0x000F0000,  "image-meta",   64          ,   "meta",     0),
        (  0x00100000,  "u-boot-fit",   640         ,   "fit",      d.expand("${DEPLOY_DIR_IMAGE}/${UBOOT_FIT_LINK}") ),
        (  0x001A0000,  "os-fit",       -1          ,   "fit",      d.expand("${DEPLOY_DIR_IMAGE}/${FIT_LINK}")),
        (  0x01C00000,  "data0",        4 * 1024    ,   "mtdonly",   None),
    )

    image_part_table_simple = (
        #  offset,      name            max_size (K)  type              src_component ( 0: fill with zero, None: skip)
        (  0,           "u-boot",       -1          , "raw",        d.expand("${DEPLOY_DIR_IMAGE}/u-boot-${MACHINE}.${UBOOT_SUFFIX}") ),
        (  0x000D0000,  "u-boot-env",   128         , "data",       0),
        (  0x000F0000,  "image-meta",   64          , "meta",       0),
        (  0x00100000,  "os-fit",       -1          , "fit",        d.expand("${DEPLOY_DIR_IMAGE}/${FIT_LINK}")),
        (  0x01C00000,  "data0",        4 * 1024    , "mtdonly",    None),
    )

    if d.getVar("VERIFIED_BOOT"):
        part_table = list(image_part_table_vboot)
    else:
        part_table = list(image_part_table_simple)

    return (image_max_size, erase_block_size, part_table)
