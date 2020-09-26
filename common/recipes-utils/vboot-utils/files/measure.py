#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
#
import argparse
import hashlib
import json
import os
import subprocess
import sys
import traceback

from image_meta import FBOBMCImageMeta
from vboot_common import EC_EXCEPTION, EC_SUCCESS, get_fdt


MBOOT_CHECK_VERSION = "1"


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


def read_tpm2_pcr(algo, pcr_id):
    if os.access("/usr/bin/tpm2_pcrread", os.X_OK):
        cmd = ["/usr/bin/tpm2_pcrread", f"{algo}:{pcr_id}"]
        tpm2_pcr_reading = subprocess.check_output(cmd).decode("utf-8")
        tpm2_pcr = bytes.fromhex(tpm2_pcr_reading.split()[3][2:])
    else:
        # fallback to TPM2 3.x tool
        cmd = ["/usr/bin/tpm2_pcrlist", "-L", f"{algo}:{pcr_id}"]
        tpm2_pcr_reading = subprocess.check_output(cmd).decode("utf-8")
        tpm2_pcr = bytes.fromhex(tpm2_pcr_reading.split()[4])
    return tpm2_pcr


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


def measure_spl(algo, image_meta):
    # PCR0 : hash(spl)
    # notice: includes the spl-dtb: keks, signing date, and lock-bit
    pcr0 = Pcr(algo)
    spl = image_meta.get_part_info("spl")
    spl_measure = hash_comp(image_meta.image, spl["offset"], spl["size"], algo)
    return pcr0.extend(spl_measure)


def measure_keystore(algo, image_meta):
    # PCR1 : hash(key-store)
    # notice: the key-store is the first 16K of the u-boot-fit
    pcr1 = Pcr(algo)
    fit = image_meta.get_part_info("u-boot-fit")
    fit_measure = hash_comp(image_meta.image, fit["offset"], 0x4000, algo)
    return pcr1.extend(fit_measure)


def get_uboot_hash_and_size(filename, offset, size):
    with open(filename, "rb") as fh:
        fh.seek(offset)
        uboot_fit = fh.read(size)
        uboot_fdt = get_fdt(uboot_fit)
        uboot_size = uboot_fdt.resolve_path("/images/firmware@1/data-size")[0]
        uboot_hash = uboot_fdt.resolve_path("/images/firmware@1/hash@1/value").to_raw()

        return (uboot_hash, uboot_size)


def measure_uboot(algo, image_meta):
    # PCR2: hash(sha256(uboot))
    # notice: to simplify SPL measure code, spl hash the "hash of uboot" read from fit
    pcr2 = Pcr(algo)
    fit = image_meta.get_part_info("u-boot-fit")
    uboot_hash, uboot_size = get_uboot_hash_and_size(
        image_meta.image, fit["offset"], 0x4000
    )

    uboot_measure = hash_comp(
        image_meta.image, fit["offset"] + 0x4000, uboot_size, "sha256"
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

    uboot_measure = hashlib.sha256(uboot_hash).digest()
    return pcr2.extend(uboot_measure)


def measure_rom_env(algo, image_meta):
    # PCR3 : hash(rom-env)
    # Notice: rom-env is the first 512 bytes of u-boot-env
    pcr3 = Pcr(algo)
    env = image_meta.get_part_info("u-boot-env")
    env_measure = hash_comp(image_meta.image, env["offset"], 0x200, algo)
    return pcr3.extend(env_measure)


def measure_uboot_env(algo, image_meta):
    # PCR5 : hash(u-boot-env)
    pcr5 = Pcr(algo)
    env = image_meta.get_part_info("u-boot-env")
    env_measure = hash_comp(image_meta.image, env["offset"], env["size"], algo)
    return pcr5.extend(env_measure)


def measure_rcv_uboot(algo, image_meta):
    # PCR2 : hash(rec-u-boot)
    # Notice: will measured when booting into golden image
    pcr2 = Pcr(algo)
    rec_uboot = image_meta.get_part_info("rec-u-boot")
    rec_uboot_measure = hash_comp(
        image_meta.image, rec_uboot["offset"], rec_uboot["size"], algo
    )
    return pcr2.extend(rec_uboot_measure)


def measure_no_uboot_env(algo, image_meta):
    # PCR3 : hash(null), data size is 0
    # Notice: Recovery U-Boot did not have u-boot-env
    pcr5 = Pcr(algo)
    env = image_meta.get_part_info("u-boot-env")
    null_uboot_measure = hash_comp(image_meta.image, env["offset"], 0, algo)
    return pcr5.extend(null_uboot_measure)


def main():
    if args.image:
        flash0_meta = FBOBMCImageMeta(args.image)
        flash1_meta = FBOBMCImageMeta(args.image)
    else:
        flash0_meta = FBOBMCImageMeta(args.flash0)
        flash1_meta = FBOBMCImageMeta(args.flash1)

    mboot_measures = [
        {  # SPL
            "component": "spl",
            "pcr_id": 0,
            "algo": args.algo,
            "expect": measure_spl(args.algo, flash0_meta).hex(),
            "measure": "NA",
        },
        {  # key-store
            "component": "key-store",
            "pcr_id": 1,
            "algo": args.algo,
            "expect": measure_keystore(args.algo, flash1_meta).hex(),
            "measure": "NA",
        },
        {  # u-boot
            "component": "u-boot",
            "pcr_id": 2,
            "algo": args.algo,
            "expect": measure_uboot(args.algo, flash1_meta).hex(),
            "measure": "NA",
        },
        {  # Recovery u-boot
            "component": "rec-u-boot",
            "pcr_id": 2,
            "algo": args.algo,
            "expect": measure_rcv_uboot(args.algo, flash0_meta).hex(),
            "measure": "NA",
        },
        {  # u-boot-env
            "component": "u-boot-env",
            "pcr_id": 3,
            "algo": args.algo,
            "expect": measure_uboot_env(args.algo, flash1_meta).hex(),
            "measure": "NA",
        },
    ]

    if args.tpm:
        for measure in mboot_measures:
            measure["measure"] = read_tpm2_pcr(measure["algo"], measure["pcr_id"]).hex()

    if args.json:
        print(json.dumps(mboot_measures))
    else:
        for measure in mboot_measures:
            print(
                f'{measure["pcr_id"]:2d}:[{measure["expect"]}] ({measure["component"]})'
            )
            print(f'   [{measure["measure"]}] (pcr{measure["pcr_id"]})')

    return EC_SUCCESS


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Measurement and compare with PCRs")
    parser.add_argument(
        "-a",
        "--algo",
        help="hash algorithm",
        choices=hashlib.algorithms_available,
        default="sha256",
    )
    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s-v{}".format(MBOOT_CHECK_VERSION),
    )
    parser.add_argument(
        "-0", "--flash0", help="flash0 device or image", default="/dev/flash0"
    )
    parser.add_argument(
        "-1", "--flash1", help="flash1 device or image", default="/dev/flash1"
    )
    parser.add_argument("-i", "--image", help="single image")
    parser.add_argument("-j", "--json", help="output as JSON", action="store_true")
    parser.add_argument("-t", "--tpm", help="read tpm pcr also", action="store_true")

    args = parser.parse_args()

    try:
        sys.exit(main())
    except Exception as e:
        print("Exception: %s" % (str(e)))
        traceback.print_exc()
        sys.exit(EC_EXCEPTION)
