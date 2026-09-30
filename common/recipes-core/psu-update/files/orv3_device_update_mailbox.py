import sys
import time
import traceback
from contextlib import contextmanager

from modbus_impl_pyrmd import ModbusCRCError, ModbusTimeout
from modbus_update_helper import print_perc, retry

# Status definitions
NORMAL_OPERATION_MODE = 0x0000
ENTERED_BOOT_MODE = 0x0001
FIRMWARE_PACKET_CORRECT = 0x0006
WAIT_STATUS = 0x0018
FIRMWARE_UPGRADE_FAILED = 0x0055
FIRMWARE_UPGRADE_SUCCESS = 0x00AA

# ORV3 BBU Register name: FW_Revision
ORV3_BBU_VERSION_REGISTER = (56, 4)

# HPR BBU Register name: FW_Revision
HPR_BBU_VERSION_REGISTER = (56, 4)

# HPR CBU Register name: FW_Revision
HPR_CBU_VERSION_REGISTER = (56, 4)

# HPR PMM Register name: PMM_FW_Revision
HPR_PMM_VERSION_REGISTER = (56, 4)

# Delta MiniUPS Version Registers
MINIUPS_Shelf_Firmware_Version = (4104, 8)
MINIUPS_Power_Module_1_Firmware_Version = (8202, 8)
MINIUPS_Power_Module_2_Firmware_Version = (8330, 8)
MINIUPS_Power_Module_3_Firmware_Version = (8458, 8)
MINIUPS_Power_Module_4_Firmware_Version = (8586, 8)
MINIUPS_Power_Module_5_Firmware_Version = (8714, 8)
MINIUPS_Power_Module_6_Firmware_Version = (8842, 8)

vendor_params = {
    "panasonic": {
        "block_size": 96,
        "boot_mode": 0xAA55,
        "block_wait": False,
        "version_regs": [ORV3_BBU_VERSION_REGISTER],
    },
    "delta": {
        "block_size": 64,
        "boot_mode": 0xA5A5,
        "block_wait": True,
        "version_regs": [ORV3_BBU_VERSION_REGISTER],
    },
    "delta_cbu": {
        "block_size": 64,
        "boot_mode": 0xA5A5,
        "block_wait": False,
        "version_regs": [HPR_CBU_VERSION_REGISTER],
    },
    "delta_miniups": {
        "block_size": 68,
        "boot_mode": 0xAA55,
        "block_wait": True,
        "version_regs": [
            MINIUPS_Shelf_Firmware_Version,
            MINIUPS_Power_Module_1_Firmware_Version,
            MINIUPS_Power_Module_2_Firmware_Version,
            MINIUPS_Power_Module_3_Firmware_Version,
            MINIUPS_Power_Module_4_Firmware_Version,
            MINIUPS_Power_Module_5_Firmware_Version,
            MINIUPS_Power_Module_6_Firmware_Version,
        ],
        "verification_time": 90.0,
    },
    "hpr_panasonic": {
        "block_size": 96,
        "boot_mode": 0xAA55,
        "block_wait": False,
        "version_regs": [HPR_BBU_VERSION_REGISTER],
        "hw_workarounds": ["FORCE_EXIT_BOOT_MODE", "FORCE_CLEAR_VERIFY"],
    },
    "hpr_pmm_panasonic": {
        "block_size": 68,
        "boot_mode": 0xAA55,
        "block_wait": True,
        "version_regs": [HPR_PMM_VERSION_REGISTER],
    },
    "hpr_pmm_delta": {
        "block_size": 68,
        "boot_mode": 0xAA55,
        "block_wait": True,
        "version_regs": [HPR_PMM_VERSION_REGISTER],
        "hw_workarounds": ["FORCE_EXIT_BOOT_MODE"],
    },
    "hpr_pmm_aei": {
        "block_size": 68,
        "boot_mode": 0xAA55,
        "block_wait": True,
        "version_regs": [HPR_PMM_VERSION_REGISTER],
        "hw_workarounds": ["FORCE_EXIT_BOOT_MODE"],
    },
}


def load_file(path):
    # File is already has bytes in 2 byte words
    # in big-endian order. So, read the data and
    # for a list of register values.
    with open(path, "rb") as f:
        ret = []
        while True:
            b = f.read(2)
            if len(b) != 2:
                break
            ret.append(int.from_bytes(b, byteorder="big"))
    return ret


@retry(5, delay=0.5)
def unlock_firmware(dev):
    dev.write(0x300, 0x55AA, timeout=1000)


@retry(15, delay=1.0)
def enter_boot_mode(dev, boot_mode):
    print("Entering Boot Mode...")
    dev.write(0x301, 0xAA55, timeout=5000)
    verify_firmware_status(dev, ENTERED_BOOT_MODE)


@retry(5, delay=1.0)
def exit_boot_mode(dev):
    print("Exiting Boot Mode...")
    try:
        dev.write(0x304, 0x55AA, timeout=10000)
    except ModbusTimeout:
        print("Exit boot mode timed out... Checking if we are in correct status")
        verify_firmware_status(dev, NORMAL_OPERATION_MODE)


@contextmanager
def boot_mode(dev, boot_mode):
    """
    Ensure we always exit boot mode
    """
    try:
        enter_boot_mode(dev, boot_mode)
        yield
    finally:
        time.sleep(10.0)
        exit_boot_mode(dev)
        time.sleep(16.0)
        verify_firmware_status(dev, NORMAL_OPERATION_MODE)


def get_firmware_status(dev):
    return dev.read(0x302, timeout=1000)[0]


def verify_firmware_status_noretry(dev, expected_status):
    # ensure 0x302 register contains expected status
    a = get_firmware_status(dev)
    if a != expected_status:
        raise ValueError(
            "Bad firmware state: 0x%02x expected: 0x%02x"
            % (int(a), int(expected_status))
        )


@retry(5, delay=1.0)
def verify_firmware_status(dev, expected_status):
    verify_firmware_status_noretry(dev, expected_status)


@retry(5, delay=1.0)
def write_block(dev, data, block_size, workarounds):
    if len(data) < block_size:
        # TODO this is a workaround with a bad .bin file
        # which can have spurious extra bytes at the end
        # which we need to ignore.
        return
    assert len(data) == block_size
    if "WRITE_BLOCK_CRC_EXPECTED" not in workarounds:
        dev.write(0x310, data, timeout=1000)
        return
    try:
        dev.write(0x310, data, timeout=1000)
    except ModbusCRCError:
        # Ignore CRC Error to support early boot-loaders
        # which respond with incorrect CRC16 code.
        # TODO remove when no longer required.
        if "PRINTED" not in workarounds:
            workarounds.append("PRINTED")
            print("WARNING: CRCError suppressed")


@retry(500, delay=0.01, verbose=0)
def wait_write_block(dev):
    verify_firmware_status_noretry(dev, FIRMWARE_PACKET_CORRECT)


def transfer_image(dev, image, block_size_words, block_wait, workarounds):
    num_words = len(image)
    sent_blocks = 0
    total_blocks = num_words // block_size_words
    if num_words % block_size_words != 0:
        total_blocks += 1
    for i in range(0, num_words, block_size_words):
        write_block(dev, image[i : i + block_size_words], block_size_words, workarounds)
        if block_wait:
            wait_write_block(dev)
        else:
            time.sleep(0.1)
        print_perc(
            sent_blocks * 100.0 / total_blocks,
            "Sending block %d of %d..." % (sent_blocks, total_blocks),
        )
        sent_blocks += 1
    print_perc(100.0, "Sending block %d of %d..." % (sent_blocks, total_blocks))


def verify_firmware(dev):
    time.sleep(10.0)
    dev.write(0x303, 0x55AA, 10000)


def workaround_force_exit_boot_mode(dev, workarounds):
    curr_mode = get_firmware_status(dev)
    if curr_mode == NORMAL_OPERATION_MODE:
        return
    print(
        "WARNING: Current firmware in status %02x. Expected %02x"
        % (curr_mode, NORMAL_OPERATION_MODE)
    )
    print("Initiating remediation")
    # Assume previous aborted upgrade. Force a verify to
    # walk it through a full abort.
    verify_firmware(dev)
    time.sleep(10.0)
    # Some devices need us to force clear the verify
    # register to walk it through completion
    if "FORCE_CLEAR_VERIFY" in workarounds:
        dev.write(0x303, 0)

    exit_boot_mode(dev)
    time.sleep(10.0)
    curr_mode = get_firmware_status(dev)
    if curr_mode != NORMAL_OPERATION_MODE:
        print("ERROR: Workaround to recover firmware from mode failed.")
        print("Current status: %02x" % (curr_mode))
        print("Continuing upgrade hoping for the best")
    unlock_firmware(dev)


def update_device(dev, filename, vendor_param):
    workarounds = vendor_param.get("hw_workarounds", [])
    verification_time = vendor_param.get("verification_time", 10.0)
    print("Parsing Firmware...")
    binimg = load_file(filename)
    print("Unlock Engineering Mode")
    unlock_firmware(dev)

    if "FORCE_EXIT_BOOT_MODE" in workarounds:
        workaround_force_exit_boot_mode(dev, workarounds)

    with boot_mode(dev, vendor_param["boot_mode"]):
        print("Transferring image")
        time.sleep(5.0)
        transfer_image(
            dev,
            binimg,
            vendor_param["block_size"] // 2,
            vendor_param["block_wait"],
            workarounds,
        )
        print("Request Verify Firmware")
        verify_firmware(dev)
        print("Waiting for verification to complete")
        time.sleep(verification_time)
        print("check firmware status")
        verify_firmware_status(dev, FIRMWARE_UPGRADE_SUCCESS)
    print("done")


@retry(5, delay=1.0)
def print_revision(dev, params):
    vers = ", ".join([dev.read_str(reg, len) for reg, len in params["version_regs"]])
    print("Version: ", vers)


def main(dev, file, vendor, block_size=None):
    params = vendor_params[vendor]
    if block_size is not None:
        params["block_size"] = block_size
    print("Upgrade Parameters: ", params)
    print_revision(dev, params)
    try:
        update_device(dev, file, params)
    except Exception as e:
        print("Firmware update failed %s" % str(e))
        traceback.print_exc()
        print("Waiting for reset....")
        time.sleep(30.0)
        sys.exit(1)
    print("Resetting....")
    time.sleep(30.0)
    print("Upgrade success")
    print_revision(dev, params)
