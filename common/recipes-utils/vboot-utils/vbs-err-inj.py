#!/usr/bin/env python3
import argparse
import mmap
import os
import struct
import sys
import traceback
from collections import namedtuple, OrderedDict


VBS_ERROR_INJECTION_VERSION = 1
SRAM_OFFSET = 0x1E720000
SRAM_SIZE = 36 * 1024
VBS_OFFSET = 0x200
VBS_SIZE = 56

EC_SUCCESS = 0
EC_EXCEPTION = 2

# struct vbs {
#   /* 00 */ uint32_t uboot_exec_address; /* Location in MMIO where U-Boot/Recovery U-Boot is execution */
#   /* 04 */ uint32_t rom_exec_address;   /* Location in MMIO where ROM is executing from */
#   /* 08 */ uint32_t rom_keys;           /* Location in MMIO where the ROM FDT is located */
#   /* 0C */ uint32_t subordinate_keys;   /* Location in MMIO where subordinate FDT is located */
#   /* 10 */ uint32_t rom_handoff;        /* Marker set when ROM is handing execution to U-Boot. */
#   /* 14 */ uint8_t force_recovery;      /* Set by ROM when recovery is requested */
#   /* 15 */ uint8_t hardware_enforce;    /* Set by ROM when WP pin of SPI0.0 is active low */
#   /* 16 */ uint8_t software_enforce;    /* Set by ROM when RW environment uses verify=yes */
#   /* 17 */ uint8_t recovery_boot;       /* Set by ROM when recovery is used */
#   /* 18 */ uint8_t recovery_retries;    /* Number of attempts to recovery from verification failure */
#   /* 19 */ uint8_t error_type;          /* Type of error, or 0 for success */
#   /* 1A */ uint8_t error_code;          /* Unique error code, or 0 for success */
#   /* 1B */ uint8_t error_tpm;           /* The last-most-recent error from the TPM. */
#   /* 1C */ uint16_t crc;                /* A CRC of the vbs structure */
#   /* 1E */ uint16_t error_tpm2;         /* The last-most-recent error from the TPM2. */
#   /* 20 */ uint32_t subordinate_last;   /* Status reporting only: the last booted subordinate. */
#   /* 24 */ uint32_t uboot_last;         /* Status reporting only: the last booted U-Boot. */
#   /* 28 */ uint32_t kernel_last;        /* Status reporting only: the last booted kernel. */
#   /* 2C */ uint32_t subordinate_current;/* Status reporting only: the current booted subordinate. */
#   /* 30 */ uint32_t uboot_current;      /* Status reporting only: the current booted U-Boot. */
#   /* 34 */ uint32_t kernel_current;     /* Status reporting only: the current booted kernel. */
# };
VBS_STRUCT = (
    "L:uboot_exec_address",
    "L:rom_exec_address",
    "L:rom_keys",
    "L:subordinate_keys",
    "L:rom_handoff",
    "?:force_recovery",
    "?:hardware_enforce",
    "?:software_enforce",
    "?:recovery_boot",
    "B:recovery_retries",
    "B:error_type",
    "B:error_code",
    "B:error_tpm",
    "H:crc",
    "H:error_tpm2",
    "L:subordinate_last",
    "L:uboot_last",
    "L:kernel_last",
    "L:subordinate_current",
    "L:uboot_current",
    "L:kernel_current",
)

VBS_PACK_STR = "<" + "".join([f.split(":")[0] for f in VBS_STRUCT])
VBS = namedtuple("vbs", [f.split(":")[1] for f in VBS_STRUCT])

# VBS Errors
VBS_ERR_DICT = {
    "VBS_SUCCESS": (0, "OpenBMC was verified correctly"),
    "VBS_ERROR_MISSING_SPI": (10, "FMC SPI0.1 (CS1) is not populated"),
    "VBS_ERROR_EXECUTE_FAILURE": (
        11,
        "U-Boot on FMC SPI0.1 (CS1) did not execute properly",
    ),
    "VBS_ERROR_SPI_PROM": (
        20,
        "FMC SPI0.1 (CS1) PROM status invalid or invalid read-mode",
    ),
    "VBS_ERROR_SPI_SWAP": (21, "SPI Swap detected"),
    "VBS_ERROR_BAD_MAGIC": (
        30,
        "Invalid FDT magic number for U-Boot FIT at 0x28080000",
    ),
    "VBS_ERROR_NO_IMAGES": (31, "U-Boot FIT did not contain the /images node"),
    "VBS_ERROR_NO_FW": (32, "U-Boot FIT /images node has no 'firmware' subnode"),
    "VBS_ERROR_NO_CONFIG": (33, "U-Boot FIT did not contain the /config node"),
    "VBS_ERROR_NO_KEK": (34, "The ROM was not built with an embedded FDT"),
    "VBS_ERROR_NO_KEYS": (35, "U-Boot FIT did not contain the /keys node"),
    "VBS_ERROR_BAD_KEYS": (
        36,
        "The intermediate keys within the U-Boot FIT are missing",
    ),
    "VBS_ERROR_BAD_FW": (37, "U-Boot data is invalid or missing"),
    "VBS_ERROR_INVALID_SIZE": (38, "U-Boot FIT total size is invalid"),
    "VBS_ERROR_KEYS_INVALID": (
        40,
        "The intermediate keys could not be verified using the ROM keys",
    ),
    "VBS_ERROR_KEYS_UNVERIFIED": (
        41,
        "The intermediate keys were not verified using the ROM keys",
    ),
    "VBS_ERROR_FW_INVALID": (
        42,
        "U-Boot could not be verified using the intermediate keys",
    ),
    "VBS_ERROR_FW_UNVERIFIED": (
        43,
        "U-Boot was not verified using the intermediate keys",
    ),
    "VBS_ERROR_FORCE_RECOVERY": (
        50,
        "Recovery boot was forced using the force_recovery environment variable",
    ),
    "VBS_ERROR_OS_INVALID": (60, "The rootfs and kernel FIT is invalid"),
    "VBS_ERROR_TPM_SETUP": (70, "There is a general TPM or TPM hardware setup error"),
    "VBS_ERROR_TPM_FAILURE": (71, "There is a general TPM API failure"),
    "VBS_ERROR_TPM_NO_PP": (72, "The TPM is not asserting physical presence"),
    "VBS_ERROR_TPM_INVALID_PP": (
        73,
        "The TPM physical presence configuration is invalid",
    ),
    "VBS_ERROR_TPM_NO_PPLL": (
        74,
        "The TPM cannot set the lifetime lock for physical presence",
    ),
    "VBS_ERROR_TPM_PP_FAILED": (75, "The TPM cannot assert physical presence"),
    "VBS_ERROR_TPM_NOT_ENABLED": (76, "The TPM is not enabled"),
    "VBS_ERROR_TPM_ACTIVATE_FAILED": (77, "The TPM cannot be activated"),
    "VBS_ERROR_TPM_RESET_NEEDED": (78, "The TPM and CPU must be reset"),
    "VBS_ERROR_TPM_NOT_ACTIVATED": (
        79,
        "The TPM was not activated after a required reset",
    ),
    "VBS_ERROR_TPM_NV_LOCK_FAILED": (80, "There is a general TPM NV storage failure"),
    "VBS_ERROR_TPM_NV_NOT_LOCKED": (81, "The TPM NV storage is not locked"),
    "VBS_ERROR_TPM_NV_SPACE": (
        82,
        "Cannot define TPM NV regions or max writes exhausted",
    ),
    "VBS_ERROR_TPM_NV_BLANK": (83, "Cannot write blank data to TPM NV region"),
    "VBS_ERROR_TPM_NV_READ_FAILED": (84, "There is a general TPM NV read failure"),
    "VBS_ERROR_TPM_NV_WRITE_FAILED": (85, "There is a general TPM NV write failure"),
    "VBS_ERROR_TPM_NV_NOTSET": (86, "The TPM NV region content is invalid"),
    "VBS_ERROR_ROLLBACK_MISSING": (90, "Rollback protection timestamp missing"),
    "VBS_ERROR_ROLLBACK_FAILED": (91, "Rollback protection failed"),
    "VBS_ERROR_ROLLBACK_HUGE": (
        92,
        "Rollback protection is jumping too far into the future",
    ),
    "VBS_ERROR_ROLLBACK_FINISH": (99, "Rollback protection did not finish"),
}


def pack_vbs(vbs: OrderedDict) -> bytearray:
    """pack the OrderedDict into VBS struct"""
    return struct.pack(VBS_PACK_STR, *vbs.values())


def unpack_vbs(vbs_data: bytearray) -> OrderedDict:
    """unpack the VBS to OrderedDict"""
    assert (
        struct.calcsize(VBS_PACK_STR) == VBS_SIZE
    ), f"vbs pack size {VBS_PACK_STR}  is not {VBS_SIZE}"
    vbs = VBS(*struct.unpack(VBS_PACK_STR, vbs_data))
    return vbs._asdict()


def write_vbs(vbs_data: bytearray):
    memfn = None
    assert (
        len(vbs_data) == VBS_SIZE
    ), f"vbs_data length {len(vbs_data)} is not {VBS_SIZE}"
    try:
        memfn = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
        with mmap.mmap(
            memfn,
            SRAM_SIZE,
            mmap.MAP_SHARED,
            mmap.PROT_READ | mmap.PROT_WRITE,
            offset=SRAM_OFFSET,
        ) as sram:
            sram.seek(VBS_OFFSET)
            sram.write(vbs_data)
    finally:
        if memfn is not None:
            os.close(memfn)


def read_vbs() -> bytearray:
    memfn = None
    try:
        memfn = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
        with mmap.mmap(
            memfn,
            SRAM_SIZE,
            mmap.MAP_SHARED,
            mmap.PROT_READ | mmap.PROT_WRITE,
            offset=SRAM_OFFSET,
        ) as sram:
            sram.seek(VBS_OFFSET)
            return sram.read(VBS_SIZE)
    finally:
        if memfn is not None:
            os.close(memfn)


def list_vbs_error_code():
    for err_name, (err_code, err_str) in VBS_ERR_DICT.items():
        print(f"{err_name:32}({err_code:2}) : {err_str}")


def print_vbs(title: str, vbs: OrderedDict):
    print(f"===={title:^32}====")
    for k, v in vbs.items():
        print(f"{k:24} = {v}")


def crc16(data: bytearray, offset, length):
    if (
        data is None
        or offset < 0
        or offset > len(data) - 1
        and offset + length > len(data)
    ):
        return 0
    crc = 0
    for i in range(0, length):
        crc ^= data[offset + i] << 8
        for _ in range(0, 8):
            if (crc & 0x8000) > 0:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
    return crc & 0xFFFF


def inject_err(
    vbs: OrderedDict, err: int, tpmerr: int = 0, tpm2err: int = 0
) -> OrderedDict:
    vbs["error_code"] = err
    vbs["error_type"] = err // 10
    vbs["error_tpm"] = tpmerr
    vbs["error_tpm2"] = tpm2err
    # prepare for recalculate CRC16
    vbs["crc"] = 0
    uboot_exec_address = vbs["uboot_exec_address"]
    vbs["uboot_exec_address"] = 0
    rom_handoff = vbs["rom_handoff"]
    vbs["rom_handoff"] = 0
    vbs["crc"] = crc16(pack_vbs(vbs), 0, VBS_SIZE)
    # put rom_handoff and uboot_exec_address back
    vbs["rom_handoff"] = rom_handoff
    vbs["uboot_exec_address"] = uboot_exec_address
    return vbs


def main() -> int:
    if args.errcode == "?":
        list_vbs_error_code()
        return 0

    vbs_data = read_vbs()
    vbs = unpack_vbs(vbs_data)
    print_vbs("Before", vbs)
    inject_err(
        vbs, err=VBS_ERR_DICT[args.errcode][0], tpmerr=args.tpm1, tpm2err=args.tpm2
    )
    print_vbs("After", vbs)
    new_vbs_data = pack_vbs(vbs)
    write_vbs(new_vbs_data)
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Verified Boot Status Error Injection")

    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s-v{}".format(VBS_ERROR_INJECTION_VERSION),
    )

    parser.add_argument(
        "errcode",
        help="error code to be injected, omit list all error code can be injected",
        nargs="?",
        default="?",
    )

    tpmerr = parser.add_mutually_exclusive_group()
    tpmerr.add_argument(
        "--tpm1",
        help="inject tpm v1 status code",
        metavar="error_tpm",
        default=0,
        type=int,
    )
    tpmerr.add_argument(
        "--tpm2",
        help="inject tpm v2 status code",
        metavar="error_tpm2",
        default=0,
        type=int,
    )

    args = parser.parse_args()

    try:
        sys.exit(main())
    except Exception as e:
        print("Exception: %s" % (str(e)))
        traceback.print_exc()
        sys.exit(EC_EXCEPTION)
