# Common code for generating u-boot FIT binary images for u-boot.
# Also generate an image for the OpenBMC ROM and upgradable flash.
#
# Copyright (C) 2020-Present, Facebook, Inc.

# Todo: switch to use yocto kernel-fitimage instead of Aspeed kernel_fitimage
inherit kernel_fitimage

python __anonymous () {
    # Todo get the version information from common place to get consistent with
    # tools: pypartition, sign-tools etc.

    # provide the image meta JSON version
    d.setVar("FBOBMC_IMAGE_META_VER_MAJOR", "1")
    d.setVar("FBOBMC_IMAGE_META_VER_MINOR", "0")
    d.setVar("FBOBMC_IMAGE_META_VER_PATCH", "0")
    d.setVar("FBOBMC_IMAGE_META_VER_PRERELEASE", "")
    d.setVar("FBOBMC_IMAGE_META_VER_BUILDMETA", "")
}

python flash_image_generate() {
    # setup the image_meta_info with version
    fbobmc_image_meta_ver = []
    fbobmc_image_meta_ver.append( int(d.getVar("FBOBMC_IMAGE_META_VER_MAJOR"),0))
    fbobmc_image_meta_ver.append( int(d.getVar("FBOBMC_IMAGE_META_VER_MINOR"),0))
    fbobmc_image_meta_ver.append( int(d.getVar("FBOBMC_IMAGE_META_VER_PATCH"),0))
    fbobmc_image_meta_ver.append( d.getVar("FBOBMC_IMAGE_META_VER_PRERELEASE") )
    fbobmc_image_meta_ver.append( d.getVar("FBOBMC_IMAGE_META_VER_BUILDMETA")  )

    image_meta_info = { "FBOBMC_IMAGE_META_VER" : tuple(fbobmc_image_meta_ver) }

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
            bb.fatal("%s %s overlapped" % curr_part, next_part)
        elif next_offset < next_part[offset] and "__END__" != next_part[name]:
            bb.fatal("gap exists between %s %s" % curr_part, next_part)
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

    bb.debug(1, "Image meta:")
    bb.debug(1, pprint.pformat(image_meta))
    today = datetime.datetime.now().strftime("%A %b %d %H:%M:%S %Z %Y")
    desc = "%s: %s" % ("build", today)
    meta_data = json.dumps(image_meta).encode("ascii")
    meta_chksum = { hashlib.md5(meta_data).hexdigest() : desc }
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


def fbobmc_generate_part_chksum(d, image_file, part_offset, part_size):
    import hashlib

    with open(image_file, "rb") as fh:
        fh.seek(part_offset)
        return hashlib.md5(fh.read(part_size)).hexdigest()


def fbobmc_generate_image_meta(d, part_table, image_meta, image_file):
    part_infos = []
    for part_offset, part_name, part_size, part_type, _ in part_table:
        partition = dict()
        partition["name"] = part_name
        partition["offset"] = part_offset
        partition["size"] = part_size
        partition["type"] = part_type
        if "meta" == part_type:
            meta_part_offset = part_offset
            meta_part_size = part_size
        if "raw" == part_type or "rom" == part_type:
            partition["chksum"] = fbobmc_generate_part_chksum(
                d, image_file, part_offset, part_size
            )
        part_infos.append(partition)

    image_meta["part_infos"] = tuple(part_infos)
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
