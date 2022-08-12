import os

if os.getenv("RUNNING_WITH_BUCK", False):
    from ncsi.ncsi import (
        get_expected_packet_size,
        make_ncsi_pkt_from_bytes,
        NCSICommand,
    )
    from ndump.ndump import get_dump_pkt_str
    from utils import utils
else:
    import utils
    from ncsi import get_expected_packet_size, make_ncsi_pkt_from_bytes, NCSICommand
    from ndump import get_dump_pkt_str

RECV_DATA_LEN = 1024


def send_packet(sock, packet, logger):
    if sock is None or sock.fileno() == -1:
        return None
    b = packet.to_bytes()
    sock.send(utils.add_length_prefix(b))


def receive_response(sock, logger):
    if sock is None or sock.fileno() == -1:
        return None
    recv_data = []
    while True:
        new_data = sock.recv(RECV_DATA_LEN)
        # check if connection is closed
        if new_data is None or len(new_data) == 0:
            return None
        recv_data.append(new_data)
        # check to see if all data was read
        if len(new_data) < RECV_DATA_LEN:
            break

    # combine array into bytes str
    data = b"".join(recv_data)

    pretty_print_str = ""
    pkt = make_ncsi_pkt_from_bytes(data)
    if pkt is None:
        return None
    pretty_print_str = get_dump_pkt_str(pkt)
    logger.debug("Received " + pretty_print_str)
    return pkt


def packet_tx_rx(sock, packet, logger):
    assert type(packet) is NCSICommand, "Packet must be a command!"

    send_packet(sock, packet, logger)  # pass socket to function\
    response = receive_response(sock, logger)
    expected_length = get_expected_packet_size(packet.type)
    assert response.length == expected_length
    assert response.type == packet.type | 0x80
    return response


def not_implemented_packet_tx_rx(sock, packet, logger):
    assert type(packet) is NCSICommand, "Packet must be a command!"

    send_packet(sock, packet, logger)  # pass socket to function
    response = receive_response(sock, logger)
    assert response.type == packet.type | 0x80
    assert response.payload_desc.response == 1  # response = 1: Command failed
    assert response.payload_desc.reason == 3  # reason = 3: channel not ready
    return response
