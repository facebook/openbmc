import socket
import struct
from collections import defaultdict

PYTEST_INSTALLED = True

try:
    import pytest  # noqa: F401
except ImportError:
    PYTEST_INSTALLED = False


def add_length_prefix(b):
    length = len(b)
    prefix = bytearray(
        [
            (length >> 24) & 0xFF,
            (length >> 16) & 0xFF,
            (length >> 8) & 0xFF,
            (length & 0xFF),
        ]
    )
    return prefix + b


def trim_length_prefix(b):
    if len(b) < 4:
        raise ValueError(f"Buffer of length {len(b)} expected to be at least 4")

    payload_length = (b[0] << 24) + (b[1] << 16) + (b[2] << 8) + b[3]
    payload = b[4:]

    if len(payload) != payload_length:
        raise ValueError(
            f"Payload length {len(payload)} != socket length {payload_length}"
        )

    return payload


def split_packet(b):
    length = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]
    return length, b[4:]


def bytearray_to_string(b):
    return " ".join(hex(n) for n in b)


def try_remove_prefix(data):
    """
    Try to remove the prefix using the pre-existing `trim_length_prefix`
    function. If it fails, then assume the length prefix of the packet
    is already trimmed.
    """
    try:
        pkt = trim_length_prefix(data)
        return pkt + struct.pack("4I", 0x0, 0x0, 0x0, 0x0)  # pad
    except ValueError:
        # Length prefix is already trimmed!
        return data


def find_free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("", 0))
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # socket.getsockname() returns an (addr, port) tuple.
        # We want just the port part.
        port = s.getsockname()[1]

    return int(port)


def pytest_installed_decorator(decorator):
    """
    If we don't have pytest installed, as is the case on the BMC,
    we do not want to use any of the pytest features.
    Therefore, we need a wrapper for pytest decorators
    to cancel them out if not needed.

    decorator is a pytest function, something along the lines of
    `pytest.fixture(foo=bar)`.
    referencing https://stackoverflow.com/a/49204061
    """

    def wrapper(function):
        if PYTEST_INSTALLED:
            return decorator(function)
        else:
            return function

    return wrapper


PACKET_TYPES = defaultdict(
    lambda: "NCSI_INVALID_COMMAND",
    {
        0x00: "NCSI_CLEAR_INITIAL_STATE",
        0x01: "NCSI_SELECT_PACKAGE",
        0x02: "NCSI_DESELECT_PACKAGE",
        0x03: "NCSI_ENABLE_CHANNEL",
        0x04: "NCSI_DISABLE_CHANNEL",
        0x05: "NCSI_RESET_CHANNEL",
        0x06: "NCSI_ENABLE_CHANNEL_NETWORK_TX",
        0x07: "NCSI_DISABLE_CHANNEL_NETWORK_TX",
        0x08: "NCSI_AEN_ENABLE",
        0x09: "NCSI_SET_LINK",
        0x0A: "NCSI_GET_LINK_STATUS",
        0x0B: "NCSI_SET_VLAN_FILTER",
        0x0C: "NCSI_ENABLE_VLAN",
        0x0D: "NCSI_DISABLE_VLAN",
        0x0E: "NCSI_SET_MAC_ADDRESS",
        0x10: "NCSI_ENABLE_BROADCAST_FILTER",
        0x11: "NCSI_DISABLE_BROADCAST_FILTER",
        0x12: "NCSI_ENABLE_MULTICAST_FILTER",
        0x13: "NCSI_DISABLE_MULTICAST_FILTER",
        0x14: "NCSI_SET_FLOW_CONTROL",
        0x15: "NCSI_GET_VERSION_ID",
        0x16: "NCSI_GET_CAPABILITIES",
        0x17: "NCSI_GET_PARAMETERS",
        0x18: "NCSI_GET_PACKET_STATISTICS",
        0x19: "NCSI_GET_NCSI_STATISTICS",
        0x1A: "NCSI_GET_PASS_THROUGH_STATISTICS",
        0x1B: "NCSI_GET_PACKAGE_STATUS",
        0x50: "NCSI_OEM_COMMAND",
        0x51: "NCSI_RESRVED1",
        0x52: "NCSI_GET_PACKAGE_UUID",
        0x53: "NCSI_PLDM",
        0x54: "NCSI_GET_SUPPORTED_MEDIA",
        0x55: "NCSI_TRANSPORT_SPECIFIC_AEN_ENABLE",
        0xFF: "NCSI_AEN_COMMAND",
    },
)


class Packet:
    def __init__(self, pkt_type):
        self.type = pkt_type
        self.payload = ()

    def to_bytes(self):
        return None

    @classmethod
    def from_bytes(cls, pkt_bytes):
        pass

    @classmethod
    def from_type(cls, pkt_type):
        return cls(pkt_type)

    def modify_payload(self):
        pass
