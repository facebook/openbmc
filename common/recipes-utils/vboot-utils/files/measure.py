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
from measure_func import (
    measure_spl,
    measure_keystore,
    measure_uboot,
    measure_rcv_uboot,
    measure_uboot_env,
    measure_blank_uboot_env,
    measure_vbs,
    measure_os,
)
from tpm_event_log import (
    TPMEventLog,
    InvalidTPMEventLog,
    MalformedTPMEventLog,
    print_tpm_event_log_debug_suggestion,
)
from vboot_common import (
    EC_EXCEPTION,
    EC_SUCCESS,
    EC_MEASURE_FAIL_BASE,
    EC_TPM_EVENT_LOG_BAD,
    VBS_ERROR_TYPE_OFFSET,
    VBS_ERROR_CODE_OFFSET,
    read_vbs,
)

MBOOT_CHECK_VERSION = "6"


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


ATTEST_COMPONENTS = [
    "spl",
    "key-store",
    "u-boot",
    "rec-u-boot",
    "os",
    "rec-os",
    "blank-u-boot-env",
]
OS_SUB_COMPNAME = ["kernel", "ramdisk", "fdt"]
RECCOVERY_OS_SUB_COMPNAME = ["rec-" + c for c in OS_SUB_COMPNAME]


def print_attest_list(fw_ver, raw_sha1_hashes, raw_sha256_hashes, comp):
    if "os" == comp or "rec-os" == comp:
        attlist = dict()
        subcomp_name = OS_SUB_COMPNAME if "os" == comp else RECCOVERY_OS_SUB_COMPNAME
        for subcomp_idx, subcomp in enumerate(subcomp_name):
            attlist[subcomp] = {
                "hashes": {
                    "sha1": raw_sha1_hashes[comp][subcomp_idx].hex(),
                    "sha256": raw_sha256_hashes[comp][subcomp_idx].hex(),
                },
                "metadata": {
                    "name": comp + "." + subcomp,
                    "version": fw_ver,
                },
                "command_lines": [],
            }
    else:
        attlist = {
            comp: {
                "hashes": {
                    "sha1": raw_sha1_hashes[comp].hex(),
                    "sha256": raw_sha256_hashes[comp].hex(),
                },
                "metadata": {
                    "name": comp,
                    "version": fw_ver,
                },
                "command_lines": [],
            },
        }

    if args.json:
        print(json.dumps(attlist))
    else:
        for c in attlist.keys():
            k = attlist[c]["metadata"]["name"] + "(sha1)"
            v = attlist[c]["hashes"]["sha1"]
            print(f"{k:27}:{v}")
            k = attlist[c]["metadata"]["name"] + "(sha256)"
            v = attlist[c]["hashes"]["sha256"]
            print(f"{k:27}:{v}")


def gen_attest_allowlists(flash0_meta, flash1_meta):
    raw_sha1_hashes = {
        "spl": measure_spl("sha1", flash0_meta, True),
        "key-store": measure_keystore("sha1", flash1_meta, True),
        "u-boot": measure_uboot("sha1", flash1_meta, args.recal, True),
        "rec-u-boot": measure_rcv_uboot("sha1", flash0_meta, True),
        "os": measure_os("sha1", flash1_meta, args.recal, True),
        "rec-os": measure_os("sha1", flash0_meta, args.recal, True),
        "blank-u-boot-env": measure_blank_uboot_env("sha1", flash1_meta, True),
    }
    raw_sha256_hashes = {
        "spl": measure_spl("sha256", flash0_meta, True),
        "key-store": measure_keystore("sha256", flash1_meta, True),
        "u-boot": measure_uboot("sha256", flash1_meta, args.recal, True),
        "rec-u-boot": measure_rcv_uboot("sha256", flash0_meta, True),
        "os": measure_os("sha256", flash1_meta, args.recal, True),
        "rec-os": measure_os("sha256", flash0_meta, args.recal, True),
        "blank-u-boot-env": measure_blank_uboot_env("sha256", flash1_meta, True),
    }
    if "ALL" in args.components:
        args.components = ATTEST_COMPONENTS

    for comp in args.components:
        print_attest_list(
            flash0_meta.meta["version_infos"]["fw_ver"],
            raw_sha1_hashes,
            raw_sha256_hashes,
            comp,
        )

    return EC_SUCCESS


def get_all_measures(flash0_meta, flash1_meta):
    return [
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
            "expect": measure_uboot(args.algo, flash1_meta, args.recal).hex(),
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
        {  # blank-u-boot-env
            "component": "blank-u-boot-env",
            "pcr_id": 3,
            "algo": args.algo,
            "expect": measure_blank_uboot_env(args.algo, flash1_meta).hex(),
            "measure": "NA",
        },
        {  # vbs
            "component": "vbs",
            "pcr_id": 5,
            "algo": args.algo,
            "expect": measure_vbs(args.algo).hex() if args.tpm else "NA",
            "measure": "NA",
        },
        {  # os
            "component": "os",
            "pcr_id": 9,
            "algo": args.algo,
            "expect": measure_os(args.algo, flash1_meta, args.recal).hex(),
            "measure": "NA",
        },
        {  # recovery-os
            "component": "recovery-os",
            "pcr_id": 9,
            "algo": args.algo,
            "expect": measure_os(args.algo, flash0_meta, args.recal).hex(),
            "measure": "NA",
        },
    ]


def get_vboot_success_measures(flash0_meta, flash1_meta):
    return [
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
            "expect": measure_uboot(args.algo, flash1_meta, args.recal).hex(),
            "measure": "NA",
        },
        {  # vbs
            "component": "vbs",
            "pcr_id": 5,
            "algo": args.algo,
            "expect": measure_vbs(args.algo).hex() if args.tpm else "NA",
            "measure": "NA",
        },
        {  # os
            "component": "os",
            "pcr_id": 9,
            "algo": args.algo,
            "expect": measure_os(args.algo, flash1_meta, args.recal).hex(),
            "measure": "NA",
        },
    ]


def get_vboot_os_invalid_measures(flash0_meta, flash1_meta):
    return [
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
            "expect": measure_uboot(args.algo, flash1_meta, args.recal).hex(),
            "measure": "NA",
        },
        {  # vbs
            "component": "vbs",
            "pcr_id": 5,
            "algo": args.algo,
            "expect": measure_vbs(args.algo).hex() if args.tpm else "NA",
            "measure": "NA",
        },
        {  # recovery-os
            "component": "recovery-os",
            "pcr_id": 9,
            "algo": args.algo,
            "expect": measure_os(args.algo, flash0_meta, args.recal).hex(),
            "measure": "NA",
        },
    ]


def get_vboot_general_fail_measures(flash0_meta, flash1_meta):
    return [
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
        {  # Recovery u-boot
            "component": "rec-u-boot",
            "pcr_id": 2,
            "algo": args.algo,
            "expect": measure_rcv_uboot(args.algo, flash0_meta).hex(),
            "measure": "NA",
        },
        {  # vbs
            "component": "vbs",
            "pcr_id": 5,
            "algo": args.algo,
            "expect": measure_vbs(args.algo).hex() if args.tpm else "NA",
            "measure": "NA",
        },
        {  # recovery-os
            "component": "recovery-os",
            "pcr_id": 9,
            "algo": args.algo,
            "expect": measure_os(args.algo, flash0_meta, args.recal).hex(),
            "measure": "NA",
        },
    ]


def get_vboot_status():
    vbs_data = read_vbs()
    return vbs_data[VBS_ERROR_TYPE_OFFSET], vbs_data[VBS_ERROR_CODE_OFFSET]


def get_need_checked_measures(flash0_meta, flash1_meta):
    # not running on BMC we are calculating expected measurement
    # of the input image
    if not args.tpm:
        return get_all_measures(flash0_meta, flash1_meta)

    # running on BMC we are doing measurement checking
    # get the measures need to check based on vboot status
    vbs_err_type, vbs_err_code = get_vboot_status()
    if vbs_err_type == 0 and vbs_err_code == 0:
        return get_vboot_success_measures(flash0_meta, flash1_meta)

    if vbs_err_type == 6 and vbs_err_code == 60:
        return get_vboot_os_invalid_measures(flash0_meta, flash1_meta)

    return get_vboot_general_fail_measures(flash0_meta, flash1_meta)


def main():
    flash0_meta = None
    flash1_meta = None
    try:
        if args.image:
            flash0_meta = FBOBMCImageMeta(args.image)
            flash1_meta = FBOBMCImageMeta(args.image)
        else:
            flash0_meta = FBOBMCImageMeta(args.flash0)
            flash1_meta = FBOBMCImageMeta(args.flash1)
    except Exception:
        # now just bypass and let the invalid flasX_meta be None
        # we will catch the error later when we need measure it
        pass

    # Only get the raw hash of components
    if args.components:
        return gen_attest_allowlists(flash0_meta, flash1_meta)

    # load TPM event log
    tpm_event_log = None
    if args.event_log != "none":
        try:
            tpm_event_log = TPMEventLog(flash0_meta, flash1_meta, args.ef)
        except Exception as e:
            if args.event_log in ["sram", "file"]:
                # fail for explicit loading
                raise e

    if tpm_event_log:
        mboot_measures = tpm_event_log.replay_measure(args.ce, args.recal)
    else:
        mboot_measures = get_need_checked_measures(flash0_meta, flash1_meta)

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

    ret = EC_SUCCESS
    if args.tpm:
        for measure in mboot_measures:
            if measure["measure"] != measure["expect"]:
                ret = EC_MEASURE_FAIL_BASE + measure["pcr_id"]

    # Print the event log
    if args.pe:
        if tpm_event_log:
            tpm_event_log.print_events(args.json)
        elif not args.json:
            print("=== TPM event log is not used ===")

    return ret


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Measurement and compare with PCRs",
        formatter_class=argparse.RawTextHelpFormatter,
    )
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
    parser.add_argument(
        "-r",
        "--recal",
        help="""recalculate hash, this is normally used on dev-server.
        Be very careful to do this on BMC, it will took more than 30 mins
        to recaludate hash for each measures.
        If you really would like to try. Please BE PATIENT!!!
        Take a coffee and check back after 30 mins.
            """,
        action="store_true",
    )
    parser.add_argument(
        "-c",
        "--components",
        help="output raw hash(measure) value of specified components",
        nargs="*",
        choices=["ALL"] + ATTEST_COMPONENTS,
    )
    parser.add_argument(
        "-e",
        "--event-log",
        nargs="?",
        choices=["sram", "file", "none", "auto"],
        default="auto",
        help="""Specify TPM event log loading mode, default [auto]:
        sram - Load TPM event log from sram at well-known SRAM location
               AST2500: 0x1E728800, AST2600: 0x10015000
        file - Load from event log binary file specified with --ef <event-file>
        auto - Load from sram if well-formed event log exists
        none - Don't load TPM event log
        """,
    )
    parser.add_argument(
        "--ef",
        help="When -e file, specify the event log file,  ignored otherwise",
    )
    parser.add_argument(
        "--pe",
        action="store_true",
        help="Print the loaded TPM event log also",
    )
    parser.add_argument(
        "--ce",
        help="Check TPM event log against image when print or use it",
        action="store_true",
    )

    args = parser.parse_args()

    if args.event_log == "file" and args.ef is None:
        parser.error("--event-log=file, please provide event file with --ef")
    else:
        args.ef = None if args.event_log != "file" else args.ef

    try:
        sys.exit(main())
    except (InvalidTPMEventLog, MalformedTPMEventLog) as e:
        print(e)
        print_tpm_event_log_debug_suggestion()
        sys.exit(EC_TPM_EVENT_LOG_BAD)
    except Exception as e:
        print("Exception: %s" % (str(e)))
        traceback.print_exc()
        sys.exit(EC_EXCEPTION)
