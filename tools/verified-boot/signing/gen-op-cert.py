#!/usr/bin/env python3

#  Copyright (c) 2023-present, META, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.

import argparse
import io
import mmap
import os
import shutil
import subprocess
import sys
import tempfile
import time
import traceback

from typing import Optional

from image_meta import FBOBMCImageMeta
from measure_func import get_uboot_hash_algo_and_size
from pyfdt import pyfdt
from sh import cpp, dtc


GEN_OP_CERT_VERSION = 1

EC_SUCCESS = 0
EC_EXCEPT = 255


OP_CERT_DTS = """
/dts-v1/;
#define OP_CERT_DTS
#include "op-cert-binding.h"
/ {{
    timestamp = <{gen_time}>;
    no-fallback;
    PROP_CERT_VER = <{cert_ver}>;
    PROP_GIU_MODE = <{giu_mode}>;
    PROP_UBOOT_HASH = [{uboot_hash}];
    PROP_UBOOT_HASH_LEN = <{uboot_hash_len}>;
}};
"""


def save_tmp_src_file(fn_tmp: str, mid_ext: str) -> None:
    if not args.debug:
        return
    dst_dir = os.path.dirname(args.output)
    dst_base_name = os.path.basename(args.output).split(".")[0]
    dst = os.path.join(dst_dir, f"{dst_base_name}.{mid_ext}.tmp")
    shutil.copy2(fn_tmp, dst)


def extract_uboot_hash(fn_img: str) -> bytearray:
    image_meta = FBOBMCImageMeta(fn_img)
    fit = image_meta.get_part_info("u-boot-fit")
    uboot_hash, uboot_hash_algo, _ = get_uboot_hash_algo_and_size(
        image_meta.image, fit["offset"], 0x4000
    )
    return bytearray(uboot_hash)


def create_cert_dtb(fn_img: str, giu_mode: str, injerr: Optional[str] = None) -> str:
    uboot_hash = extract_uboot_hash(fn_img)
    if injerr == "hash":
        uboot_hash[0] ^= 1
    with tempfile.NamedTemporaryFile() as tmp_cert_dts_raw, \
         tempfile.NamedTemporaryFile() as tmp_cert_dts, \
         tempfile.NamedTemporaryFile(delete=False) as tmp_cert_dtb:  # fmt:skip
        cert_dts = OP_CERT_DTS.format(
            gen_time=hex(int(time.time())),
            cert_ver=(
                "VBOOT_OP_CERT_VER"
                if injerr != "ver"
                else "VBOOT_OP_CERT_UNSUPPORT_VER"
            ),
            giu_mode=(giu_mode if injerr != "mode" else 0xEE),
            uboot_hash=bytes(uboot_hash).hex(),
            uboot_hash_len=len(uboot_hash),
        )
        tmp_cert_dts_raw.write(cert_dts.encode("utf-8"))
        tmp_cert_dts_raw.flush()
        save_tmp_src_file(tmp_cert_dts_raw.name, "raw")
        cpp(
            "-nostdinc",
            "-undef",
            "-x",
            "assembler-with-cpp",
            "-I",
            os.path.dirname(os.path.realpath(__file__)),
            tmp_cert_dts_raw.name,
            tmp_cert_dts.name,
        )
        save_tmp_src_file(tmp_cert_dts.name, "cpp")
        dtc(
            "-I",
            "dts",
            "-O",
            "dtb",
            "-o",
            tmp_cert_dtb.name,
            tmp_cert_dts.name,
        )

    return tmp_cert_dtb.name


OP_CERT_ITS = """
/dts-v1/;
/ {{
    description = "vboot op-cert file";
    images {{
        fdt@1 {{
            description = "vboot operation certificate";
            data = /incbin/("{cert_dtb}");
            hash@1 {{
                algo = "{hash_algo}";
            }};
            signature@1 {{
                algo = "sha256,rsa4096";
                key-name-hint = "{key_name}";
            }};
        }};
    }};
    configurations {{
        default = "conf@1";
        conf@1 {{
            firmware = "fdt@1";
        }};
    }};
}};
"""
CERT_SIGN_PATH = "/images/fdt@1/signature@1/value"


def create_cert_itb(
    mkimage: str,
    hsmkey: Optional[str],
    keyfile: Optional[str],
    hash_algo: str,
    tmp_cert_dtb_name: str,
    fn_cert: str,
) -> None:
    if hsmkey:
        requested_key_name = os.path.basename(hsmkey)
        keydir = os.path.dirname(hsmkey)
    else:
        keybase, keyext = os.path.splitext(keyfile)
        if keyext != ".key":
            raise ValueError(f"private key file {keyfile} must be .key ext")
        requested_key_name = os.path.basename(keybase)
        keydir = os.path.dirname(os.path.abspath(keyfile))
    with tempfile.NamedTemporaryFile() as tmp_cert_its:
        cert_its = OP_CERT_ITS.format(
            cert_dtb=tmp_cert_dtb_name,
            hash_algo=hash_algo,
            key_name=requested_key_name,
        )
        print(cert_its)
        tmp_cert_its.write(cert_its.encode("utf-8"))
        tmp_cert_its.flush()
        save_tmp_src_file(tmp_cert_its.name, "its")
        cmd = [
            mkimage,
            "-f",
            tmp_cert_its.name,
            "-k",
            keydir,
            "-r",
            fn_cert,
        ]
        if hsmkey:
            cmd += ["-N", "FB-HSM"]
        print(" ".join(cmd))
        subprocess.run(cmd, check=True)


def decompile_dtb_file(fn_dtb: str, fn_src: str) -> None:
    dtc("-I", "dtb", "-O", "dts", "-o", fn_src, fn_dtb)


def get_cert_sign(fn_cert: str) -> bytes:
    with open(fn_cert, "rb") as fh:
        cert_io = io.BytesIO(fh.read())
        cert_fdt = pyfdt.FdtBlobParse(cert_io).to_fdt()
        return cert_fdt.resolve_path(CERT_SIGN_PATH).to_raw()


class CertSignatureNotFind(Exception):
    pass


def flip_bit_of_sign(fn_cert: str) -> None:
    cert_sig = get_cert_sign(fn_cert)
    with open(fn_cert, "r+b") as fh:
        with mmap.mmap(fh.fileno(), 0) as mm:
            sig_idx = mm.find(cert_sig)
            if sig_idx < 0:
                raise CertSignatureNotFind
            mm[sig_idx] ^= 1
            mm.flush()


def main(args: argparse.Namespace) -> int:
    # Create the certificate data from OP_CERT_DTS
    tmp_cert_dtb_name = None
    try:
        tmp_cert_dtb_name = create_cert_dtb(args.image, args.giu_mode, args.injerr)
        create_cert_itb(
            args.mkimage,
            args.hsmkey,
            args.keyfile,
            args.hash_algo,
            tmp_cert_dtb_name,
            args.output,
        )
        dump_dir = os.path.dirname(args.output)
        dump_base_name = os.path.basename(args.output).split(".")[0]
        dump_base_path = os.path.join(dump_dir, dump_base_name)
        if args.injerr == "sig":
            if args.debug:
                shutil.copy2(args.output, f"{dump_base_path}.orig.itb")
                decompile_dtb_file(args.output, f"{dump_base_path}.orig.its")
            flip_bit_of_sign(args.output)
        if args.debug:
            decompile_dtb_file(tmp_cert_dtb_name, f"{dump_base_path}.dts")
            decompile_dtb_file(args.output, f"{dump_base_path}.its")
    finally:
        if tmp_cert_dtb_name:
            os.unlink(tmp_cert_dtb_name)
    return EC_SUCCESS


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate a vboot operation certificate file."
    )
    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s-v{}".format(GEN_OP_CERT_VERSION),
    )
    parser.add_argument(
        "--mkimage",
        required=True,
        metavar="MKIMAGE",
        help="Required path to mkimage, use openbmc built mkimage for HSM sign",
    )
    parser.add_argument(
        "-i",
        "--image",
        required=True,
        help="Required openbmc image the certifate bound to",
    )
    parser.add_argument(
        "output",
        metavar="CERT_FILE",
        help="Output path of signed certificate file",
    )
    parser.add_argument(
        "-m",
        "--giu-mode",
        default="GIU_CERT",
        choices=["GIU_NONE", "GIU_CERT", "GIU_OPEN"],
        help="Golden image mode",
    )
    parser.add_argument(
        "--hash-algo",
        default="sha256",
        help="Specify hashing algorithm, default(sha256)",
    )
    parser.add_argument(
        "-d",
        "--debug",
        action="store_true",
        help="save dts and its in same dir of output cert with same basename",
    )
    parser.add_argument(
        "--injerr",
        choices=["sig", "mode", "ver", "hash"],
        help="generate bad certificate with errors for testing",
    )

    pkey = parser.add_mutually_exclusive_group(required=True)
    pkey.add_argument(
        "--keyfile",
        help="certificate signing private key file must with .key ext",
    )
    pkey.add_argument(
        "--hsmkey",
        help="Use HSM based key to sign",
    )

    args = parser.parse_args()
    # sanity check and normalize the input keydir
    try:
        sys.exit(main(args))
    except Exception as e:
        print("Exception: %s" % (str(e)))
        traceback.print_exc()
        sys.exit(EC_EXCEPT)
