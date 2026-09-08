import os
import struct
import sys
import time
import traceback

from modbus_update_helper import print_perc

BLOCK_SIZE = 192

# How long the component takes to come back on the new image. Until then it
# either does not answer or still reports the version it is replacing.
REBOOT_SECS = 8.0

IMAGE_SUFFIX = ".tar.gz"

# vers_reg is the (register, num_registers) of the component's FW version
# string as defined in the orv3_rpu2 rackmon register map.
AALCV2_COMPONENTS = {
    "PUMP_RACK_ETH": {
        "name": "MT-E_P.tar.gz",
        "vers_reg": (0x1A3, 4),
    },
    "PUMP_RACK_RPU": {
        "name": "MT-R_P.tar.gz",
        "vers_reg": (0x19F, 4),
    },
    "PUMP_RACK_UPSCOM": {
        "name": "UPSCOM_P.tar.gz",
        "vers_reg": (0x90F5, 5),
    },
    "PUMP_RACK_UPSINV": {
        "name": "UPSINV_P.tar.gz",
        "vers_reg": (0x90F0, 5),
    },
    "PUMP_RACK_UPSPFC": {
        "name": "UPSPFC_P.tar.gz",
        "vers_reg": (0x90EB, 5),
    },
    "FAN_RACK_1_ETH": {
        "name": "MT-E_F1.tar.gz",
        "vers_reg": (0x1AB, 4),
    },
    "FAN_RACK_1_RPU": {
        "name": "MT-R_F1.tar.gz",
        "vers_reg": (0x1A7, 4),
    },
    "FAN_RACK_1_UPSCOM": {
        "name": "UPSCOM_F1.tar.gz",
        "vers_reg": (0x9132, 5),
    },
    "FAN_RACK_1_UPSINV": {
        "name": "UPSINV_F1.tar.gz",
        "vers_reg": (0x912D, 5),
    },
    "FAN_RACK_1_UPSPFC": {
        "name": "UPSPFC_F1.tar.gz",
        "vers_reg": (0x9128, 5),
    },
    "FAN_RACK_2_ETH": {
        "name": "MT-E_F2.tar.gz",
        "vers_reg": (0x1B3, 4),
    },
    "FAN_RACK_2_RPU": {
        "name": "MT-R_F2.tar.gz",
        "vers_reg": (0x1AF, 4),
    },
    "FAN_RACK_2_UPSCOM": {
        "name": "UPSCOM_F2.tar.gz",
        "vers_reg": (0x9174, 5),
    },
    "FAN_RACK_2_UPSINV": {
        "name": "UPSINV_F2.tar.gz",
        "vers_reg": (0x916F, 5),
    },
    "FAN_RACK_2_UPSPFC": {
        "name": "UPSPFC_F2.tar.gz",
        "vers_reg": (0x916A, 5),
    },
}

# The name the device is told to expect -> the component it belongs to.
COMPONENT_BY_IMAGE_NAME = {
    info["name"]: comp for comp, info in AALCV2_COMPONENTS.items()
}


class Status:
    status_map = {
        "ACK": 0xAC,
        "NACK_ERR_TCRC": 0xDF,
        "ERR_SYSTEM": 0xA1,
        "ERR_UNKNOWDATA": 0xB2,
        "ERR_UNKNOWHEAD": 0xC3,
        "ERR_OUTOFLEN": 0xD4,
        "ERR_ENDLEN": 0xE5,
        "ERR_ENDCRC": 0xF6,
        "ERR_BUSY": 0xBB,
    }

    def __init__(self, val):
        self.val = val

    def __str__(self):
        for k, v in self.status_map.items():
            if v == self.val:
                return k
        return "%02x" % (self.val)

    def check(self, cmd):
        if self.val == 0xAC:
            return
        raise ValueError("Failed command %s : %s" % (cmd, str(self)))


def get_rpu_revision(dev, component):
    reg, num = AALCV2_COMPONENTS[component]["vers_reg"]
    return dev.read_str(reg, num)


def parse_file_path(path):
    """
    Images are named <component>_<target>_<version>.tar.gz, for example
    MT-E_F1_1.2.3.tar.gz. Return the component the name describes. The version
    in the name is not expected to match what the device reports, so it is
    ignored.
    """
    name = os.path.basename(path)
    if not name.endswith(IMAGE_SUFFIX):
        raise ValueError(f"{path} is not the expected tar.gz")
    # The version may itself contain underscores, the two fields before it
    # may not, so stop splitting once they are consumed.
    parts = name[: -len(IMAGE_SUFFIX)].split("_", 2)
    if len(parts) != 3:
        raise ValueError(f"{path} does not contain the 3 parts in name")
    component, target, _version = parts
    minimal_name = component + "_" + target + IMAGE_SUFFIX
    if minimal_name not in COMPONENT_BY_IMAGE_NAME:
        raise ValueError(f"Unknown component {component} or target {target}")
    return COMPONENT_BY_IMAGE_NAME[minimal_name]


def upgrade_unlock(dev):
    req = b"\x64\x01\x19\x00\x01"
    resp = dev.raw(req, expected=8, timeout=1000)
    if resp != req:
        raise ValueError("Upgrade unlock failed")


def rpu_command(dev, cmd, data=b""):
    req = struct.pack(">BBB", 0x65, cmd, len(data)) + data
    resp = dev.raw(req, expected=6, timeout=5000)
    func, rcmd, state = struct.unpack(">BBB", resp)
    if func != 0x65 or rcmd != cmd:
        raise ValueError("RPU Command failed")
    return Status(state)


def rpu_data(dev, block, data):
    req = struct.pack(">BHH", 0x66, block, len(data)) + data
    resp = dev.raw(req, expected=7, timeout=5000)
    func, rblock, state = struct.unpack(">BHB", resp)
    if func != 0x66 or rblock != block:
        raise ValueError(f"RPU Data for block {block} failed")
    # TODO probably raise on this as well.
    return Status(state)


def file_target(dev, fname):
    rpu_command(dev, 0x02, fname.encode("utf-8")).check("target")


def file_binlen(dev, blen):
    return rpu_command(dev, 0x04, struct.pack("<I", blen)).check("binlen")


def file_bincrc(dev, bcrc):
    return rpu_command(dev, 0x06, struct.pack("<I", bcrc)).check("bincrc")


def file_blocklen(dev, blklen):
    return rpu_command(dev, 0x6F, struct.pack("<H", blklen)).check("blocklen")


def file_block_start(dev, numblks):
    return rpu_command(dev, 0x1A, struct.pack("<H", numblks)).check("blockstart")


def file_block_end(dev):
    return rpu_command(dev, 0x4D).check("blockend")


def crc32mpeg2(buf):
    crc = 0xFFFFFFFF
    for val in buf:
        val = int(val)
        crc = (crc ^ (val << 24)) & 0xFFFFFFFF
        for _ in range(8):
            crc = (
                crc << 1 if (crc & 0x80000000) == 0 else (crc << 1) ^ 0x4C11DB7
            ) & 0xFFFFFFFF
    return crc


def parse_image(ipath):
    blocks = []
    size = 0
    crc = 0
    with open(ipath, "rb") as f:
        data = f.read()
        size = len(data)
        crc = crc32mpeg2(data)
        for start in range(0, size, BLOCK_SIZE):
            rem = len(data) - start
            if rem <= BLOCK_SIZE:
                block = data[start:]
            else:
                block = data[start : start + BLOCK_SIZE]
            blocks.append(block)
    return blocks, size, crc


def send_image(dev, blocks):
    num_blocks = len(blocks)
    for block_num in range(0, num_blocks):
        rpu_data(dev, block_num, blocks[block_num]).check(f"data-block{block_num}")
        print_perc(
            block_num * 100.0 / num_blocks,
            "Sending block %d of %d..." % (block_num, num_blocks),
        )
    print_perc(
        100.0,
        "Sending block %d of %d..." % (num_blocks, num_blocks),
    )


def update_rpu(dev, image, component):
    image_name = AALCV2_COMPONENTS[component]["name"]
    print("Current Version: %s" % (get_rpu_revision(dev, component)))
    blocks, size, crc = parse_image(image)
    upgrade_unlock(dev)
    file_target(dev, image_name)
    file_binlen(dev, size)
    file_bincrc(dev, crc)
    file_blocklen(dev, BLOCK_SIZE)
    file_block_start(dev, len(blocks))
    send_image(dev, blocks)
    file_block_end(dev)
    time.sleep(REBOOT_SECS)
    print("Version After Upgrade: %s" % (get_rpu_revision(dev, component)))


def main(dev, file, component=None):
    try:
        if component is None:
            component = parse_file_path(file)
        update_rpu(dev, file, component)
    except Exception:
        print("Update Failed")
        traceback.print_exc()
        sys.exit(1)
