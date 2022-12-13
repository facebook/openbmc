# Common code for generating u-boot FIT binary images for u-boot.
# Also generate an image for the OpenBMC ROM and upgradable flash.
#
# Copyright (C) 2020-Present, Facebook, Inc.

# Todo: switch to use yocto kernel-fitimage instead of Aspeed kernel_fitimage
inherit kernel_fitimage

# Default Golden Image size is 32 MB
FBVBOOT_GOLDEN_IMAGE_SIZE_MB ??= "32"

python __anonymous () {
    # Todo get the version information from common place to get consistent with
    # tools: pypartition, sign-tools etc.

    # provide the image meta JSON version
    d.setVar("FBOBMC_IMAGE_META_VER", "1")

    # meta partiton offset
    d.setVar("FBOBMC_IMAGE_META_OFFSET", "0x000F0000")

    # sanity check and set FBVBOOT_GOLDEN_IMAGE_SIZE
    golden_image_size_cfg = d.getVar("FBVBOOT_GOLDEN_IMAGE_SIZE_MB", True)
    golden_image_size_mb = int(golden_image_size_cfg, 0)
    if golden_image_size_mb not in [32, 64]:
        bb.fatal("FBVBOOT_GOLDEN_IMAGE_SIZE_MB(%s) not support" % golden_image_size_cfg)
}

python flash_image_generate() {
    # setup the image_meta_info with version
    image_meta_info = { "FBOBMC_IMAGE_META_VER" : int(d.getVar("FBOBMC_IMAGE_META_VER"),0) }

    fbobmc_generate_image(d, image_meta_info)
}

def fbobmc_print_part_table(part_table):
    for part_offset, part_name, part_size, part_type, part_comp in part_table:
        part_name = "'" + part_name + "'"
        part_type = "'" + part_type + "'"
        bb.debug(
            1,
            "( 0x%08X %-12s 0x%08X %-8s [%s])"
            % (part_offset, part_name, part_size, part_type, part_comp),
        )


def fbobmc_get_normalized_part_table(d):
    offset = 0
    name = 1
    size = 2

    image_max_size, erase_block_size, raw_part_table = fbobmc_get_image_part_table(d)
    raw_part_table.sort(key=lambda part: part[offset])
    if raw_part_table[0][offset] != 0:
        bb.fatal("first partiton %s offset is not 0" % raw_part_table[0])
    # to simplify the checking logic add a "__END__" dummy partition
    dummy_end_part = (image_max_size, "__END__", 0, "dummy", None)
    raw_part_table.append(dummy_end_part)
    nom_part_table = []
    for i in range(len(raw_part_table) - 1):
        curr_part = list(raw_part_table[i])
        next_part = raw_part_table[i + 1]
        # check offset alignement
        if curr_part[offset] % erase_block_size:
            bb.fatal(
                "%s offset is not align with erase block size 0x%X"
                % (curr_part, erase_block_size)
            )
        # normalize size in bytes
        if curr_part[size] == -1:
            curr_part[size] = next_part[offset] - curr_part[offset]
        else:
            curr_part[size] *= 1024
        # check the parition overlap or gap
        next_offset = curr_part[offset] + curr_part[size]
        if next_offset > next_part[offset]:
            bb.fatal("%s %s overlapped" % (curr_part, next_part))
        elif next_offset < next_part[offset] and "__END__" != next_part[name]:
            bb.fatal("gap exists between %s %s" % (curr_part, next_part))
        else:
            nom_part_table.append(tuple(curr_part))

    fbobmc_print_part_table(nom_part_table)
    return tuple(nom_part_table)


def fbobmc_generate_image_according(d, part_table, image_file):
    import struct

    bb.debug(1, "Creating flash image [%s]" % image_file)
    with open(image_file, "wb") as fh:
        for part_offset, part_name, part_size, _, part_comp in part_table:
            if part_comp is None:
                continue
            if part_offset != fh.tell():
                bb.fatal(
                    "image write not aligned current_pos=%d, expect_pos=%d"
                    % (fh.tell(), part_offset)
                )
            elif type(part_comp) is int:
                fill_byte = struct.pack("B", part_comp)
                bb.debug(
                    1,
                    "Fill %s [0x%08X - 0x%08X] with %s"
                    % (part_name, fh.tell(), fh.tell() + part_size - 1, fill_byte),
                )
                fh.write(fill_byte * part_size)
            else:
                comp_size = os.stat(part_comp).st_size
                if comp_size > part_size:
                    bb.fatal(
                        "[%s] size (%d) is too big for %s with max size %d "
                        % (part_comp, comp_size, part_name, part_size)
                    )
                bb.debug(
                    1,
                    "write [%s] (%d) to %s [0x%08X - 0x%08X] padding with 0"
                    % (
                        part_comp,
                        comp_size,
                        part_name,
                        fh.tell(),
                        fh.tell() + part_size - 1,
                    ),
                )
                with open(part_comp, "rb") as comp_fh:
                    fh.write(comp_fh.read())
                if part_name != "os-fit":
                    fh.write(b"\0" * (part_size - comp_size))


def fbobmc_save_meta_data(d, image_meta, image_file, meta_part_offset, meta_part_size):
    import json, hashlib, datetime, pprint

    image_meta["meta_update_action"] = "build"
    image_meta["meta_update_time"] = datetime.datetime.utcnow().isoformat()
    bb.debug(1, "Image meta:")
    bb.debug(1, pprint.pformat(image_meta, indent=4))
    meta_data = json.dumps(image_meta).encode("ascii")
    meta_chksum = { "meta_md5": hashlib.md5(meta_data).hexdigest() }
    bb.debug(1, "meta checksum: ")
    bb.debug(1, pprint.pformat(meta_chksum))
    meta_chksum_data = json.dumps(meta_chksum).encode("ascii")
    with open(image_file, "r+b") as fh:
        fh.seek(meta_part_offset)
        fh.write(meta_data + b"\n")
        fh.write(meta_chksum_data + b"\n")
        if fh.tell() > (meta_part_offset + meta_part_size):
            bb.fatal(
                "%s is too bigger to save in meta part (0x%08X %d)"
                % (image_meta, meta_part_offset, meta_part_size)
            )


def fbobmc_generate_part_md5(d, image_file, part_offset, part_size):
    import hashlib

    with open(image_file, "rb") as fh:
        fh.seek(part_offset)
        return hashlib.md5(fh.read(part_size)).hexdigest()

# search all printable string with 8+ length
def fbobmc_extract_strings_from_binary(data):
    import re
    regexp = b"[\x20-\x7E]{8,}"
    str_pattern = re.compile(regexp)
    return [s.decode("ascii") for s in str_pattern.findall(data)]

# The safest way is parser the version info from uboot binary
# although we can get the value from data-store
# OPENBMC_VERSION and PREFERRED_VERSION_u-boot
def fbobmc_parser_version_infos_from_first_part(d, uboot):
    import re
    version_infos = {}
    version_infos["fw_ver"] = "unknown"
    version_infos["uboot_ver"] = "unknown"
    version_infos["uboot_build_time"] = "unknown"
    uboot_strs = []
    with open(uboot, "rb") as fh:
        data = fh.read()
        uboot_strs = fbobmc_extract_strings_from_binary(data)

    uboot_ver_regex = [
        r"^U-Boot",  # Leading
        r"(SPL )?(?P<uboot_ver>20\d{2}\.\d{2})",  # uboot_ver
        r"(?P<fw_ver>[^ ]+)",  # fw_ver
        r"\((?P<uboot_build_time>[^)]+)\).*$",  # bld_time
    ]

    uboot_ver_re = re.compile(" ".join(uboot_ver_regex))
    for uboot_str in uboot_strs:
        matched = uboot_ver_re.match(uboot_str)
        if matched:
            version_infos["fw_ver"] = matched.group("fw_ver")
            version_infos["uboot_ver"] = matched.group("uboot_ver")
            version_infos["uboot_build_time"] = matched.group("uboot_build_time")
            break
    return version_infos

def fbobmc_generate_image_meta(d, part_table, image_meta, image_file):
    fixed_meta_partition_offset = int(d.expand("${FBOBMC_IMAGE_META_OFFSET}"), 0)
    part_infos = []
    for part_offset, part_name, part_size, part_type, _ in part_table:
        partition = dict()
        partition["name"] = part_name
        partition["offset"] = part_offset
        partition["size"] = part_size
        partition["type"] = part_type
        # omit mtdonly partiton
        if "mtdonly" == part_type:
            continue
        # sanity check the meta partition offset
        if "meta" == part_type:
            meta_part_offset = part_offset
            meta_part_size = part_size
            if meta_part_offset != fixed_meta_partition_offset:
                bb.fatal("The 'meta' partition shall be fixed at 0x%08X",
                    fixed_meta_partition_offset)
        # Add calculate md5 for raw or rom type partition
        if "raw" == part_type or "rom" == part_type:
            partition["md5"] = fbobmc_generate_part_md5(
                d, image_file, part_offset, part_size
            )
        # Add num of subnodes for FIT partition, now we hardcoded
        # for os-fit subnode number is 3: kernel, rootfs and dtb
        # for u-boot-fit subnode number is 1: u-boot only.
        if "fit" == part_type:
            if "os-fit" == part_name:
                partition["num-nodes"] = 3 # kernel, rootfs, dtb
            elif "u-boot-fit" == part_name:
                partition["num-nodes"] = 1 # u-boot
            else:
                bb.fatal("unknown FIT partition name %s" % part_name)
        part_infos.append(partition)

    image_meta["part_infos"] = tuple(part_infos)
    first_part_file = part_table[0][4]
    image_meta["version_infos"] = fbobmc_parser_version_infos_from_first_part(d, first_part_file)
    fbobmc_save_meta_data(d, image_meta, image_file, meta_part_offset, meta_part_size)


def fbobmc_generate_image(d, image_meta):
    image_file = d.expand("${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE}")
    if image_file is None:
        bb.fatal("Not define image file name")

    bb.debug(
        1,
        "generate image [%s] with meta ver %s"
        % (image_file, image_meta["FBOBMC_IMAGE_META_VER"]),
    )
    part_table = fbobmc_get_normalized_part_table(d)
    fbobmc_generate_image_according(d, part_table, image_file)
    fbobmc_generate_image_meta(d, part_table, image_meta, image_file)
    # create the link
    image_file_link = d.expand("${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE_LINK}")
    image_file_link_target = d.expand("${FLASH_IMAGE}")
    bb.debug(1, "Create symlink [%s -> %s]" % (image_file_link, image_file_link_target))
    try:
        os.remove(image_file_link)
    except OSError:
        pass
    os.symlink(image_file_link_target, image_file_link)

def fbobmc_get_default_image_part_table(d, flash_size_mb=128, data0_size_mb=32):
    # default flash size is 128MB, data0 size is 32MB
    # define the partition table of image with (offset, name, max_size)
    # the max_size is used to do sanity check of the partition define
    # it can be omitted as -1, which will be calculated automatically
    # the order in the partition in the list does not matter
    # sanity check will make sure the partitions not overlap, no gaps
    # align with erase_block_size and fits in the space defined as image_max_size
    image_max_size = flash_size_mb << 20
    erase_block_size = 64 * 1024
    # FBVBOOT_GOLDEN_IMAGE_SIZE define in unit of MB
    image_end = int(d.getVar("FBVBOOT_GOLDEN_IMAGE_SIZE_MB", True), 0) << 20
    # put data0 at end of device
    data0_size_k = data0_size_mb << 10
    data0_begin = image_max_size - (data0_size_k << 10)

    image_part_table_simple = (
        #  offset,      name            max_size (K)  type              src_component ( 0: fill with zero, None: skip)
        (  0,           "u-boot",       -1          , "raw",        d.expand("${DEPLOY_DIR_IMAGE}/u-boot-${MACHINE}.${UBOOT_SUFFIX}") ),
        (  0x000E0000,  "u-boot-env",   64          , "data",       0),
        (  0x000F0000,  "image-meta",   64          , "meta",       0),
        (  0x00100000,  "os-fit",       -1          , "fit",        d.expand("${DEPLOY_DIR_IMAGE}/${FIT_LINK}")),
        (  image_end,   "spare",        -1          , "mtdonly",    None),
        (  data0_begin, "data0",        data0_size_k, "mtdonly",    None),
    )

    image_part_table_vboot = (
        #  offset,      name            max_size (K)    type            src_component ( 0: fill with zero, None: skip)
        (  0,           "spl",          256         ,   "rom",      d.expand("${DEPLOY_DIR_IMAGE}/u-boot-spl-${MACHINE}.${UBOOT_SUFFIX}") ),
        (  0x00040000,  "rec-u-boot",   640         ,   "raw",      d.expand("${DEPLOY_DIR_IMAGE}/u-boot-recovery-${MACHINE}.${UBOOT_SUFFIX}")),
        (  0x000E0000,  "u-boot-env",   64          ,   "data",     0),
        (  0x000F0000,  "image-meta",   64          ,   "meta",     0),
        (  0x00100000,  "u-boot-fit",   640         ,   "fit",      d.expand("${DEPLOY_DIR_IMAGE}/${UBOOT_FIT_LINK}") ),
        (  0x001A0000,  "os-fit",       -1          ,   "fit",      d.expand("${DEPLOY_DIR_IMAGE}/${FIT_LINK}")),
        (  image_end,   "spare",        -1          , "mtdonly",    None),
        (  data0_begin, "data0",        data0_size_k, "mtdonly",    None),
    )

    if d.getVar("VERIFIED_BOOT"):
        part_table = list(image_part_table_vboot)
    else:
        part_table = list(image_part_table_simple)

    return (image_max_size, erase_block_size, part_table)
