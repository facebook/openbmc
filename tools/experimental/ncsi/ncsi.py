import os
import struct
from collections import defaultdict, namedtuple
from ctypes import c_uint32
from enum import auto, Enum
from itertools import zip_longest

RUNNING_WITH_BUCK = os.getenv("RUNNING_WITH_BUCK", False)

if RUNNING_WITH_BUCK:
    import utils.utils as utils
else:
    import utils

NCSI_ETH_TYPE = 0x88F8
PADDING_LEN = 6
device_mac = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]
ctrl_mac = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]

NCSI_ETH_TYPE = 0x88F8

NCSI_AEN_PACKET = 0xFF
NCSI_CLEAR_INITIAL_STATE = 0x00
NCSI_SELECT_PACKAGE = 0x01
NCSI_DESELECT_PACKAGE = 0x02
NCSI_ENABLE_CHANNEL = 0x03
NCSI_DISABLE_CHANNEL = 0x04
NCSI_RESET_CHANNEL = 0x05
NCSI_ENABLE_CHANNEL_NETWORK_TX = 0x06
NCSI_DISABLE_CHANNEL_NETWORK_TX = 0x07
NCSI_AEN_ENABLE = 0x08
NCSI_SET_LINK = 0x09
NCSI_GET_LINK_STATUS = 0x0A
NCSI_SET_VLAN_FILTER = 0x0B
NCSI_ENABLE_VLAN = 0x0C
NCSI_DISABLE_VLAN = 0x0D
NCSI_SET_MAC_ADDRESS = 0x0E
# There is no packet type 0xF.
NCSI_ENABLE_BROADCAST_FILTER = 0x10
NCSI_DISABLE_BROADCAST_FILTER = 0x11
NCSI_ENABLE_MULTICAST_FILTER = 0x12
NCSI_DISABLE_MULTICAST_FILTER = 0x13
NCSI_SET_FLOW_CONTROL = 0x14
NCSI_GET_VERSION_ID = 0x15
NCSI_GET_CAPABILITIES = 0x16
NCSI_GET_PARAMETERS = 0x17
NCSI_GET_PACKET_STATISTICS = 0x18
NCSI_GET_NCSI_STATISTICS = 0x19
NCSI_GET_PASS_THROUGH_STATISTICS = 0x1A
NCSI_GET_PACKAGE_STATUS = 0x1B


NCSI_OEM_COMMAND = 0x50  # used to trigger AENs
NCSI_RESRVED1 = 0x51
NCSI_GET_PACKAGE_UUID = 0x52
NCSI_PLDM = 0x53
NCSI_GET_SUPPORTED_MEDIA = 0x54
NCSI_TRANSPORT_SPECIFIC_AEN_ENABLE = 0x55

Pd = namedtuple("PacketDesc", "length, fmt, members")

response_codes = defaultdict(
    lambda: "Meta-specific response",
    {
        0x0: "Command Completed",
        0x1: "Command Failed",
        0x2: "Command Unavailable",
        0x3: "Command Unsupported",
    },
)

reason_codes = defaultdict(
    lambda: "Meta-specific response",
    {
        0x0000: "No Error/No Reason",
        0x0001: "Interface Initialization Required",
        0x0002: "Parameter Is Invalid, Unsupported, or Out-of Range",
        0x0003: "Channel Not Ready",
        0x0004: "Package Not Ready",
        0x0005: "Invalid payload length",
        0x7FFF: "Unknown/Unsupported Command Type",
    },
)

# {0: 'Link Status Change',
# 1: 'Configuration Required',
# 2: 'Host NC Driver Status Change',
# 3: 'Reserved', ...,
# 119: 'Reserved }"
aen_type_to_description = defaultdict(
    lambda: "Meta-specific response",
    {
        # unpack basic AEN types into a new dict
        **{
            0: "Link Status Change",
            1: "Configuration Required",
            2: "Host NC Driver Status Change",
        },
        # create a dict mapping range of 0x3-0x79 to "Reserved"
        # and store into the same dict as the basic AEN types
        **dict(zip(range(0x3, 0x79), 0x75 * ["Reserved"])),
    },
)

special_commands = defaultdict(
    lambda: Pd(0, "", []),  # most commands do not have a payload.
    {
        NCSI_SELECT_PACKAGE: Pd(4, "!3xB", ["hardware_arb_disable"]),
        NCSI_DISABLE_CHANNEL: Pd(4, "!3xB", ["ald"]),
        NCSI_AEN_ENABLE: Pd(8, "!3xBI", ["aen_mc_id", "aen_ctrl"]),
        NCSI_SET_LINK: Pd(8, "!II", ["link_settings", "oem_link_settings"]),
        NCSI_SET_VLAN_FILTER: Pd(
            8, "!2xH 2xBB", ["vlan_id", "filter_selector", "enable"]
        ),
        NCSI_SET_MAC_ADDRESS: Pd(
            8,
            "!8B",
            [
                "mac_byte_5",
                "mac_byte_4",
                "mac_byte_3",
                "mac_byte_2",
                "mac_byte_1",
                "mac_byte_0",
                "mac_addr_num",
                "at_e",
            ],
        ),
        NCSI_OEM_COMMAND: Pd(4, "!3xB", ["aen_type"]),
    },
)

special_responses = defaultdict(
    lambda: Pd(4, "!hh", ["response", "reason"]),  # default value
    # special responses
    {
        NCSI_GET_LINK_STATUS: Pd(
            16, "!hhIII", ["response", "reason", "status", "other", "oem_status"]
        ),
        NCSI_GET_VERSION_ID: Pd(
            40,
            "!hhI3xB12sI4hI",
            [
                "response",
                "reason",
                "ncsi_version",
                "alpha2",
                "fw_name",
                "fw_version",
                "pci_did",
                "pci_vid",
                "pci_ssid",
                "pci_svid",
                "mf_id",
            ],
        ),
        NCSI_GET_CAPABILITIES: Pd(
            32,
            "!hhIIIIIBBBB2xBB",
            [
                "response",
                "reason",
                "cap",
                "bc_cap",
                "mc_cap",
                "buf_cap",
                "aen_cap",
                "vlan_cnt",
                "mixed_cnt",
                "mc_cnt",
                "uc_cnt",
                "vlan_mode",
                "channel_cnt",
            ],
        ),
        NCSI_GET_PACKET_STATISTICS: Pd(
            168,
            "!hh 41I",
            [
                "response",
                "reason",
                "clr_hi",
                "clr_lo",
                "rx_bytes",
                "tx_bytes",
                "rx_uni",
                "rx_mutli",
                "rx_bc",
                "tx_uni",
                "tx_multi",
                "tx_bc",
                "fcx_rx_errs",
                "align_errs",
                "fc_errs",
                "runt_pkts",
                "jabber_pkts",
                "rx_p_xon",
                "rx_p_xoff",
                "tx_p_xon",
                "tx_p_xoff",
                "tx_single_col",
                "tx_mutli_col",
                "late_col",
                "exc_col",
                "ctrl",
                "f64b_rx",
                "f65_127b_rx",
                "f128_255b_rx",
                "f256_511b_rx",
                "f512_1023b_rx",
                "f1024_1522b_rx",
                "f1523_9022b_rx",
                "f64b_tx",
                "f65_127b_tx",
                "f128_255b_tx",
                "f256_511b_tx",
                "f512_1023b_tx",
                "f1024_1522b_tx",
                "f1523_9022b_tx",
                "valid_b_rx",
                "err_runt_pkts_rx",
                "err_jabber_pkts_rx",
            ],
        ),
        NCSI_GET_PARAMETERS: Pd(
            32,
            "!hh B2xBBxHIIIBB2xI",
            [
                "response",
                "reason",
                "mac_addr_cnt",
                "mac_addr_flags",
                "vlan_tag_cnt",
                "vlan_tag_flags",
                "link_settings",
                "bx_p_filter_settings",
                "cf_flags",
                "vlan_mode",
                "flow_ctrl_en",
                "aen_ctrl",
            ],
        ),  # TODO add variable length fields for MAC addr and vlan tags!
        NCSI_GET_NCSI_STATISTICS: Pd(
            32,
            "!hh 7I",
            [
                "response",
                "reason",
                "ncsi_cmds_rx",
                "ncsi_ctrl_p_dropped",
                "ncsi_cmd_type_errs",
                "ncsi_cmd_chcksm_errs",
                "ncsi_rx_pkts",
                "ncsi_tx_pkts",
                "aens_sent",
            ],
        ),
        NCSI_GET_PASS_THROUGH_STATISTICS: Pd(
            48,
            "!hh Q9I",
            [
                "response",
                "reason",
                "tx_pkts_received",
                "tx_pkts_dropped",
                "tx_pkt_ch_state_errs",
                "tx_pkt_undersized_errs",
                "tx_pkt_oversized_errs",
                "rx_pkts_lan_interface",
                "rx_pkts_dropped",
                "rx_pkt_ch_state_errs",
                "rx_pkt_undersized_errs",
                "rx_pkt_oversized_errs",
            ],
        ),
        NCSI_OEM_COMMAND: Pd(8, "!hh I", ["response", "reason", "mf_id"]),
    },
)


def get_cmd_codes_and_names(cmds):
    """
    This returns a list of tuples (cmd_code, cmd_name) for the given
    command argument. `cmds` will be a list of strings that argparse
    generated, but the list may be either of hex values
    (such as ["0x00, 0x15"]) or actual command names
    (such as ["clear_initial_state", "select_package"]).
    We need to account for both scenarios.
    """

    # {"NCSI_COMMAND_NAME":NCSI_COMMAND_CODE} mapping
    packet_codes = {v: k for k, v in utils.PACKET_TYPES.items()}

    commands = []
    for cmd in cmds:
        if cmd.lower().startswith("0x"):
            # treat command as a hex number!
            cmd_code = int(cmd, base=16)
            cmd_name = utils.PACKET_TYPES[cmd_code]
            commands.append((cmd_code, cmd_name))
        else:
            # treat command as a sttring!
            cmd_name = ("ncsi_" + cmd).upper().strip()
            try:
                cmd_code = packet_codes[cmd_name]
                commands.append((cmd_code, cmd_name))
            except KeyError:
                raise KeyError("Not a valid NCSI command!") from None

    return commands


def get_expected_packet_size(packet):
    return special_responses[packet].length


# https://stackoverflow.com/questions/5289069/identify-the-number-of-elements-in-a-python-struct-pack-fmt-string
def unpack_to_zeros(fmt):
    entries = struct.unpack(fmt, bytes(struct.calcsize(fmt)))
    return entries


def parse_payload_to_tuple(payload, fmt, members, pkt_is_initialized=False):
    Pl = namedtuple("Payload", members)
    try:
        parsed = Pl(*payload)
        return parsed
    except ValueError as e:
        raise struct.error(
            f"Malformed packet: payload malformed ({e.__class__})"
        ) from e


def unpack_err_check(fmt, buf, offset, name, is_payload=False):
    try:
        res = struct.unpack_from(fmt, buf, offset)
        return res
    except struct.error as e:
        raise struct.error(
            f"Malformed packet: {name} malformed ({e.__class__})"
        ) from None


def is_ncsi_request(type):
    return type & 0x80 == 0


def is_ncsi_response(type):
    return type & 0x80 == 0x80


def make_ncsi_pkt_from_bytes(pkt_bytes):
    """
    Factory method to
    make NCSI packet with parameters filled from byte fields.
    """
    if pkt_bytes is None:
        raise ValueError("Must provide packet bytes!")

    packet = None
    pkt_bytes = utils.try_remove_prefix(pkt_bytes)

    dst = unpack_err_check("6B", pkt_bytes, 0, "dst")
    src = unpack_err_check("6B", pkt_bytes, 6, "src")
    eth = unpack_err_check("!H", pkt_bytes, 12, "eth")

    # unpack returns a tuple. Take the first element
    if eth[0] != NCSI_ETH_TYPE:
        return None  # not an NCSI packet

    mcid, rev, iid = unpack_err_check("BBxB", pkt_bytes, 14, "contents")
    type, channel = unpack_err_check("!2B", pkt_bytes, 18, "type/channel")

    if type == NCSI_AEN_PACKET:
        packet = AENPacket(0)

        packet.length = 0x4  # per the spec, AEN packet payload length is always 4.

        # aen type is at byte 19 (+14 bytes eth header),
        # but this unpacks the reserved bytes as well
        # that start at byte 16
        # only one value in tuple!
        packet.aen_type = unpack_err_check("!3xB", pkt_bytes, 30, "aen type")[0]
        # payload starts at byte 20 (+14 bytes eth header)
        packet.payload = unpack_err_check("!I", pkt_bytes, 34, "payload")
        packet.aen_description = aen_type_to_description[packet.aen_type]

    else:
        if is_ncsi_request(type):
            packet = NCSICommand(0)
            packet_desc = special_commands[(type) & 0x7F]

        else:
            packet = NCSIResponse(0)
            packet_desc = special_responses[(type) & 0x7F]

        packet.length = struct.calcsize(packet_desc.fmt)
        payload = unpack_err_check(
            packet_desc.fmt, pkt_bytes, 30, "payload", is_payload=True
        )
        packet.payload = payload
        packet.payload_desc = parse_payload_to_tuple(
            payload, packet_desc.fmt, packet_desc.members
        )
        packet.packet_desc = packet_desc

    packet.dst = dst
    packet.src = src
    packet.eth = eth
    packet.rev = rev
    packet.iid = iid
    packet.mcid = mcid
    packet.type = type
    packet.channel = channel
    return packet


def calculate_ncsi_checksum(pkt_bytes):
    """
    Referencing
    https://gitlab.freedesktop.org/slirp/libslirp/-/blob/master/src/ncsi.c#L41-56
    """
    checksum = 0
    # we need to convert pkt bytes into an array of uint16's
    # iterate through every pair of bytes
    # equivalent to for loop (i=0, j=1; i<len, j<len,i+=2,j+=2)
    for idx, next_idx in zip_longest(pkt_bytes[0::2], pkt_bytes[1::2], fillvalue=0):
        checksum += (idx << 8) + next_idx

    checksum = ~checksum + 1
    # convert to uint32
    return c_uint32(checksum).value


class NCSIPacket(utils.Packet):
    id = 0

    def __init__(self, pkt_type):
        """
        Constructor of generic packet object
        """
        super().__init__(pkt_type)

        if type is None:
            raise ValueError("Must provide packet type!")

        iid = NCSIPacket.id
        NCSIPacket.id += 1
        self.iid = iid

        # NCSI packet format varies based on if the
        # packet is a request or a response
        if is_ncsi_request(pkt_type):
            packet_desc = special_commands[(pkt_type) & 0x7F]
        else:
            packet_desc = special_responses[(pkt_type) & 0x7F]

        self.packet_desc = packet_desc
        self.length = struct.calcsize(packet_desc.fmt)
        self.payload = struct.unpack(
            packet_desc.fmt, bytes(struct.calcsize(packet_desc.fmt))
        )
        self.payload_desc = parse_payload_to_tuple(
            self.payload, packet_desc.fmt, packet_desc.members
        )
        self.channel = 0x0

        self.mcid = 0
        self.dst = device_mac
        self.src = ctrl_mac
        self.eth = NCSI_ETH_TYPE
        self.rev = 0x1

    def to_bytes(self):
        prefix = (
            struct.pack("6B", *device_mac)
            + struct.pack("6B", *ctrl_mac)
            + struct.pack("!H", 0x88F8)
        )
        packet_info = (
            struct.pack("!BBxB", self.mcid, self.rev, self.iid)
            + struct.pack("!2B", self.type, self.channel)
            # payload length is 12 bit. The most significant 4 bits are reserved.
            # needs to be little-endian
            + struct.pack(">H", self.length & 0x0FFF)
            + struct.pack("HHHH", 0, 0, 0, 0)  # 4 halves of padding
        )
        payload = struct.pack(self.packet_desc.fmt, *self.payload_desc)

        pkt = prefix + packet_info + payload
        checksum = calculate_ncsi_checksum(pkt)

        return pkt + struct.pack("!I", checksum)

    def modify_payload(self, attr_new_attr_mapping):
        """
        `pkt` is an NCSICommand or NCSIResponse
        `attr_new_attr_mapping` is a dict of attribute --> new value
        to set packet payload attributes to.
        """
        assert (
            type(attr_new_attr_mapping) is dict
        ), "`attr_new_attr_mapping` must be a dictionary!"

        pkt_format = self.packet_desc.fmt
        pkt_members = self.packet_desc.members
        Pl = namedtuple("Payload", pkt_members)

        # parse payload into tuple according to the format
        tup = Pl(*(self.payload_desc))

        # validate attr_new_attr_mapping types are the same as existing
        for attr, new_attr in attr_new_attr_mapping.items():
            # get the current attribute value
            # may throw AttributeError if field does not exist
            try:
                current_attr_val = getattr(tup, attr)
            except AttributeError:
                raise AttributeError(
                    f"Field {attr} does not exist in payload!"
                ) from None

            # validate the types are the same
            if type(current_attr_val) != type(new_attr):
                raise ValueError(f"Type of {attr} is incompatiable!")

        try:

            # set the attribute to the new value
            tup = tup._replace(**attr_new_attr_mapping)
            self.payload_desc = tup

            # re-pack the payload
            self.payload = struct.pack(pkt_format, *tup)
            return self.payload
        except struct.error as e:
            raise ValueError("Cannot modify payload!") from e

    def dump(self):
        return self.__dict__


class NCSICommand(NCSIPacket):
    id = 0

    def __init__(self, pkt_type):
        """
        Constructor of command object
        """
        super().__init__(pkt_type)


class NCSIResponse(NCSIPacket):
    id = 0

    def __init__(self, pkt_type):
        """
        Constructor of response packet object
        """
        super().__init__(pkt_type | 0x80)


class AENPacket(NCSIPacket):
    def __init__(self, aen_type):
        """
        Constructor of AEN packet object
        """
        super().__init__(NCSI_AEN_PACKET)
        self.aen_type = aen_type
        self.aen_description = aen_type_to_description[self.aen_type]
        self.payload = (0x0,)

    def to_bytes(self):
        prefix = (
            struct.pack("6B", *device_mac)
            + struct.pack("6B", *ctrl_mac)
            + struct.pack("!H", NCSI_ETH_TYPE)
        )
        packet_info = (
            struct.pack(
                "!BBxB", self.mcid, self.rev, self.iid
            )  # MCID = 0, rev = 1, reserved, iid = 0
            + struct.pack(
                "!BBxB", self.type, self.channel, self.length
            )  # control packet type, orig. channel id, reserved, payload length = 4
            + struct.pack("2I", 0, 0)  # 8 bytes reserved
            + struct.pack("!3xB", self.aen_type)
        )
        payload = struct.pack("!I", *self.payload)
        pkt = prefix + packet_info + payload
        checksum = calculate_ncsi_checksum(pkt)

        return pkt + struct.pack("!I", checksum)


class StateMachineEnum(Enum):
    # auto-assign values -- only care about the names for now.
    POWER_OFF = auto()
    PACKAGE_CHANNEL_NOT_READY = auto()
    PACKAGE_DESELECTED_ALL_CHANNELS_IN_INITIAL_STATE = auto()
    PACKAGE_SELECTED_CHANNEL_IN_INITIAL_STATE = auto()
    PACKAGE_SELECTED_CHANNEL_READY = auto()
    PACKAGE_DESELECTED_CHANNEL_READY = auto()


class PowerEnum(Enum):
    POWER_DOWN = 0
    POWER_UP = 1
