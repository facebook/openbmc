import logging
import os

RUNNING_WITH_BUCK = os.getenv("RUNNING_WITH_BUCK", False)
if RUNNING_WITH_BUCK:
    from ncsi import ncsi
    from utils import utils
else:
    import ncsi
    import utils

logger = logging.getLogger("ndump")


def parse_payload(pkt):
    payload_format = pkt.packet_desc
    payload_fields = payload_format.members

    # try:
    # payload starts at byte 30 -- DSP0222 section 8.4.2
    values = pkt.payload

    response_dict = dict(zip(payload_fields, values))
    if pkt.type & 0x80:  # all response packets have a response/reason
        response_code = response_dict["response"]
        reason_code = response_dict["reason"]
        response_dict["response"] = (
            str(response_code) + f" ({ncsi.response_codes[response_code]})"
        )
        response_dict["reason"] = (
            str(reason_code) + f" ({ncsi.reason_codes[reason_code]})"
        )

    return response_dict


def get_dump_pkt_str(pkt):
    """
        Get the pretty-print of a packet object.
    `"""
    if pkt is None:
        return  # not an NCSI packet

    pkt_type = type(pkt)
    dump_str = f"{pkt_type.__name__}: "
    dump_str += f"""MC ID: {hex(pkt.mcid)}, Rev: {hex(pkt.rev)}, IID: {hex(pkt.iid)}
            Type: {hex(pkt.type)}, Type code: {utils.PACKET_TYPES[pkt.type & 0x7F]}
            Channel: {hex(pkt.channel)}"""

    if pkt_type == ncsi.AENPacket:
        aen_type = pkt.aen_type
        dump_str += f"""
            AEN type: {hex(aen_type)} ({ncsi.aen_type_to_description[aen_type]})
            """

    else:
        if pkt.payload_desc is not None:
            payload = parse_payload(pkt)
            payload_str = ""
            for field, value in payload.items():
                if type(value) is not bytes and type(value) is not str:
                    value = hex(value)
                payload_str += f"""
                \t{field}: {value}"""

            dump_str += f"""
                Payload length: {pkt.length}
                Payload:{payload_str}
                """

    return dump_str


def dump_packet(pkt_bytes, logger):
    """
    Verbose packet example:

    NCSI
        MC ID: 0x00
        Revision: 0x01
        IID: 0x01
        Type: 0x02, Type code: Deselect Package, Type req/resp: request
            .000 0010 = Type code: Deselect Package (0x02)
            0... .... = Type req/resp: request (0x0)
        Channel: 0x1f
            000. .... = Package ID: 0x0
            ...1 1111 = Internal Channel ID: 0x1f
        Payload Length: 0x00
    """
    pkt = ncsi.make_ncsi_pkt_from_bytes(pkt_bytes)
    dump_str = get_dump_pkt_str(pkt)

    logger.debug(dump_str)
