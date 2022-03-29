# Copyright 2022-present Facebook. All Rights Reserved.
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
import hashlib

from vboot_common import (
    get_fdt,
    get_fdt_from_file,
    read_vbs,
)


class Pcr:
    def __init__(self, algo="sha256"):
        self.algo = algo
        h = hashlib.new(self.algo)
        self.value = b"\0" * h.digest_size
        self.block_size = h.block_size
        self.digest_size = h.digest_size

    def extend(self, value):
        h = hashlib.new(self.algo, self.value)
        h.update(value)
        self.value = h.digest()
        return self.value


def hash_comp(filename, offset, size, algo="sha256"):
    h = hashlib.new(algo)
    with open(filename, "rb") as fh:
        fh.seek(offset)
        remain = size
        while remain:
            read_size = min(remain, h.block_size)
            data = fh.read(read_size)
            remain -= read_size
            h.update(data)
    return h.digest()


def measure_spl(algo, image_meta, rawhash=False):
    # PCR0 : hash(spl)
    # notice: includes the spl-dtb: keks, signing date, and lock-bit
    pcr0 = Pcr(algo)
    spl = image_meta.get_part_info("spl")
    spl_measure = hash_comp(image_meta.image, spl["offset"], spl["size"], algo)
    return spl_measure if rawhash else pcr0.extend(spl_measure)


def measure_keystore(algo, image_meta, rawhash=False):
    # PCR1 : hash(key-store)
    # notice: the key-store is the first 16K of the u-boot-fit
    pcr1 = Pcr(algo)
    fit = image_meta.get_part_info("u-boot-fit")
    fit_measure = hash_comp(image_meta.image, fit["offset"], 0x4000, algo)
    return fit_measure if rawhash else pcr1.extend(fit_measure)


def get_uboot_hash_algo_and_size(filename, offset, size):
    with open(filename, "rb") as fh:
        fh.seek(offset)
        uboot_fit = fh.read(size)
        uboot_fdt = get_fdt(uboot_fit)
        uboot_size = uboot_fdt.resolve_path("/images/firmware@1/data-size")[0]
        uboot_hash = uboot_fdt.resolve_path("/images/firmware@1/hash@1/value").to_raw()
        uboot_algo = uboot_fdt.resolve_path("/images/firmware@1/hash@1/algo")[0]

        return (uboot_hash, uboot_algo, uboot_size)


def get_os_comps_hash_algo_offset_size(filename, offset, size):
    comps = []

    with open(filename, "rb") as fh:
        fh.seek(offset)
        os_fit = get_fdt_from_file(fh)
        # kernel
        kernel_hash = os_fit.resolve_path("/images/kernel@1/hash@1/value").to_raw()
        kernel_algo = os_fit.resolve_path("/images/kernel@1/hash@1/algo")[0]
        kernel_data = os_fit.resolve_path("/images/kernel@1/data")
        comps.append(
            (
                kernel_hash,
                kernel_algo,
                kernel_data.blob_info[0],
                kernel_data.blob_info[1],
            )
        )
        # ramdisk
        ramdisk_hash = os_fit.resolve_path("/images/ramdisk@1/hash@1/value").to_raw()
        ramdisk_algo = os_fit.resolve_path("/images/ramdisk@1/hash@1/algo")[0]
        ramdisk_data = os_fit.resolve_path("/images/ramdisk@1/data")
        comps.append(
            (
                ramdisk_hash,
                ramdisk_algo,
                ramdisk_data.blob_info[0],
                ramdisk_data.blob_info[1],
            )
        )
        # fdt
        fdt_hash = os_fit.resolve_path("/images/fdt@1/hash@1/value").to_raw()
        fdt_algo = os_fit.resolve_path("/images/fdt@1/hash@1/algo")[0]
        fdt_data = os_fit.resolve_path("/images/fdt@1/data")
        comps.append((fdt_hash, fdt_algo, fdt_data.blob_info[0], fdt_data.blob_info[1]))

    return comps


def measure_uboot(algo, image_meta, recal=False, rawhash=False):
    # PCR2: hash(sha256(uboot))
    # notice: to simplify SPL measure code, spl hash the "hash of uboot" read from fit
    pcr2 = Pcr(algo)
    fit = image_meta.get_part_info("u-boot-fit")
    uboot_hash, uboot_hash_algo, uboot_size = get_uboot_hash_algo_and_size(
        image_meta.image, fit["offset"], 0x4000
    )

    if recal:
        # uboot_hash_algo define in FIT signature which is independent with
        # measure (input algo of hash_comp) and PCR hash algorithm
        uboot_measure = hash_comp(
            image_meta.image, fit["offset"] + 0x4000, uboot_size, uboot_hash_algo
        )
        assert (
            uboot_hash == uboot_measure
        ), """Build, Signing or SPL code BUG!!!.
            UBoot size:0x%08X.
            Hash:
                expected:[%s]
                measured:[%s]
            """ % (
            uboot_size,
            uboot_hash.hex(),
            uboot_measure.hex(),
        )

    uboot_measure = hashlib.new(algo, uboot_hash).digest()
    return uboot_measure if rawhash else pcr2.extend(uboot_measure)


def measure_uboot_env(algo, image_meta, rawhash=False):
    # PCR3 : hash(u-boot-env)
    pcr3 = Pcr(algo)
    env = image_meta.get_part_info("u-boot-env")
    env_measure = hash_comp(image_meta.image, env["offset"], env["size"], algo)
    return env_measure if rawhash else pcr3.extend(env_measure)


def measure_rcv_uboot(algo, image_meta, rawhash=False):
    # PCR2 : hash(rec-u-boot)
    # Notice: will measured when booting into golden image
    pcr2 = Pcr(algo)
    rec_uboot = image_meta.get_part_info("rec-u-boot")
    rec_uboot_measure = hash_comp(
        image_meta.image, rec_uboot["offset"], rec_uboot["size"], algo
    )
    return rec_uboot_measure if rawhash else pcr2.extend(rec_uboot_measure)


def measure_blank_uboot_env(algo, image_meta, rawhash=False):
    # PCR3 : hash(64KB zero)
    # Notice: in the image the uboot env partition is blank
    # during the first time boot up u-boot will save the
    # default u-boot environment into this blank env partition.
    # But SPL measure the uboot env partition before this
    # so SPL will measure a blank uboot env during the first time
    # bootup. Add it here to help test cover this special case.
    pcr3 = Pcr(algo)
    blank_uboot_env_measure = hashlib.new(algo, bytearray(64 * 1024)).digest()
    return blank_uboot_env_measure if rawhash else pcr3.extend(blank_uboot_env_measure)


def measure_os(algo, image_meta, recal=False, rawhash=False):
    # PCR9: hash(sha256(kernel)), hash(sha256(rootfs)), hash(sha256(fdt))
    # notice: to simplify the measure code, we extend
    #         hash of the sha256(kernel),
    #         hash of the sha256(rootfs),
    #         hash of the sha256(fdt)
    # in ORDER into PCR9.
    pcr9 = Pcr(algo)
    fit = image_meta.get_part_info("os-fit")
    os_components = get_os_comps_hash_algo_offset_size(
        image_meta.image, fit["offset"], fit["size"]
    )

    raw_hashes = []
    for comp_hash, comp_hash_algo, comp_offset, comp_size in os_components:
        if recal:
            # comp_hash_algo define in FIT signture which is independent with
            # measure (input algo of hash_comp) and PCR hash algorithm
            comp_measure = hash_comp(
                image_meta.image, comp_offset, comp_size, comp_hash_algo
            )
            assert (
                comp_hash == comp_measure
            ), """Build, Signing or uboot code BUG!!!.
                size:0x%08X.
                Hash:
                expected:[%s]
                measured:[%s]
            """ % (
                comp_size,
                comp_hash.hex(),
                comp_measure.hex(),
            )
        comp_measure = hashlib.new(algo, comp_hash).digest()
        raw_hashes += [comp_measure]
        pcr9.extend(comp_measure)

    return raw_hashes if rawhash else pcr9.value


def measure_vbs(algo, rawhash=False):
    # PCR5: vbs structure
    pcr5 = Pcr(algo)
    vbs_measure = hashlib.new(algo, read_vbs()).digest()
    return vbs_measure if rawhash else pcr5.extend(vbs_measure)
