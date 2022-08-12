import argparse
import logging
import os
import socket
import time
from socket import htons


RUNNING_WITH_BUCK = os.getenv("RUNNING_WITH_BUCK", False)
MAX_CONN_RETRIES = 10
RECV_DATA_LEN = 1024

if RUNNING_WITH_BUCK:
    from ncsi import ncsi
    from ncsi_transport.ncsi_transport import receive_response, send_packet
    from ndump.ndump import get_dump_pkt_str
else:
    import ncsi
    from ncsi_transport import receive_response, send_packet
    from ndump import get_dump_pkt_str


logger = logging.getLogger("MOCK")


class MockTerminus:
    def __init__(self, addr=("127.0.0.1", 5555), interface=None, listen_only=False):
        self.addr = addr
        self.interface = interface
        if self.interface is None:  # make tcp socket
            self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        else:
            # check interface is valid.
            if interface not in os.listdir("/sys/class/net/"):
                raise ValueError("Interface not found!")

            # check for right permissions.
            # to bind to a device, need to be root.
            # os.geteuid returns 0 if running with elevated priv
            if os.geteuid() != 0:
                raise OSError("Need to be root to run!")

            host, port = self.addr
            # from <linux/if_ether.h>
            # #define ETH_P_ALL	0x0003		/* Every packet (be careful!!!) */
            self._sock = socket.socket(
                socket.AF_PACKET, socket.SOCK_RAW, proto=htons(0x3)
            )
            # https://stackoverflow.com/a/7221783
            # Linux's SO_BINDTODEVICE is defined as 25
            interface_as_bytes = str(interface).encode("utf-8")
            self._sock.setsockopt(socket.SOL_SOCKET, 25, interface_as_bytes)
            self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            self.addr = (str(interface), 0)
        self._prev_resp = None
        self.listen_only = listen_only
        # initialized later, on client connection
        self._client_conn = None
        self._client_addr = None

    def start(self):
        # self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        logger.info("===================================")
        logger.info("RUNNING WITH MOCK")
        logger.info("===================================\n\n")

        retries = 0
        while retries < MAX_CONN_RETRIES:
            try:
                self._sock.bind(self.addr)
                return self

            except OSError:
                logger.warning(
                    f"Failed to bind, trying again in 1 second...(Retries: {retries})"
                )
                retries += 1
                time.sleep(2)
        if retries == MAX_CONN_RETRIES:
            logger.error(f"Failed to bind mock to addr, {self.addr}")
            raise RuntimeError(f"Failed to bind mock to addr, {self.addr}")

    def shutdown(self):
        if self._sock is not None and self._sock.fileno() != -1:
            self._sock.close()
        self.disconnect_conn()

    def power_on(self):
        self.start()

    def power_off(self):
        self.shutdown()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self.power_off()

    def establish_conn(self):
        """
        Check if connection needs to be established, or do
        nothing if conn is already established.

        Returns: (socket connection, socket address) tuple
        """
        # No need to re-establish connection if
        # it is already active
        if self._client_conn is not None and self._client_conn.fileno() != -1:
            return (self._client_conn, self._client_addr)
        # establish new connection
        if self.interface is None and self._sock.fileno() != -1:
            """
            socket.listen() essentially creates a socket server on the address it is
            bound to and accept() accepts a connection on that address
            If the address is an interface, however, these two commands fail because
            it cannot start a server on an interface!

            We only call listen() and accept() only if  we are testing via inet-style
            connections. Otherwise, recv() is sufficient.
            """
            self._client_conn, self._client_addr = self._sock.accept()
        else:
            conn = self._sock
            self._client_conn, self._client_addr = conn, self.interface
        return (self._client_conn, self._client_addr)

    def disconnect_conn(self):
        logger.info(f"Closing connection to {self._client_addr}")
        if self._client_conn is not None:
            self._client_conn.close()
            self._client_conn = None
        self._client_addr = None

    def listen(self, verbose=True):
        logger.info(f"Listening on {str(self.addr)}")
        if self.interface is None:
            self._sock.listen(8)

        while True:
            conn, addr = self.establish_conn()
            # consider making this non-blocking?
            self.__recv_pkt_and_resp(conn, verbose)

    def __recv_pkt_and_resp(self, conn, verbose):

        pkt = receive_response(conn, logger)
        if pkt is None:
            return

        if not self.listen_only:
            # check if pkt is valid

            # assume a packet with command code 0x50 is a request to generate AEN packet
            # as per T119218681 description
            resp = None
            if pkt.type == ncsi.NCSI_OEM_COMMAND:
                resp = self.__create_aen_pkt(pkt.payload_desc.aen_type)

            else:
                resp = self.__create_response_pkt(pkt)  # respond
            if resp is not None:
                # save packet, as per DSP0222 spec requirements

                self._prev_resp = resp

                if verbose:
                    resp_pkt_str = get_dump_pkt_str(resp)
                    logger.debug("Responding with: \n" + resp_pkt_str)
                send_packet(conn, resp, logger)

    def __create_response_pkt(self, pkt):
        """
        Respond to a packet, based on the current state of the controller.
        """

        resp_pkt = ncsi.NCSIResponse(pkt.type | 0x80)
        if resp_pkt.type == (ncsi.NCSI_GET_VERSION_ID | 0x80):
            # claim that mock is a mellanox nic.
            # 0x8119 is mellanox mfr id
            resp_pkt.modify_payload({"mf_id": 0x8119})
        return resp_pkt

    def __create_aen_pkt(self, aen_type):
        resp_pkt = ncsi.AENPacket(aen_type)
        return resp_pkt

    async def __schedule_aen_pkt(self, pkt, delay):
        pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--host",
        default="localhost",
        help="Host Address to receive packets on. Defaults to `localhost`. \
        Not applicable if attaching to interface.",
    )
    parser.add_argument(
        "--port",
        default="5555",
        type=int,
        help="Port to listen on. Defaults to `5555`. \
        Not applicable if attaching to interface.",
    )

    parser.add_argument(
        "-v", "--verbose", action="count", default=1, help="Verbosity (-v, -vv, etc)"
    )

    parser.add_argument(
        "-e",
        "--echo",
        action="store_false",
        default=True,
        help="Whether or not to reply to incoming messages",
    )
    parser.add_argument(
        "-i",
        "--interface",
        default=None,
        help="Interface to bind to (such as tap0, eth0, etc)",
    )

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)
    with MockTerminus(
        addr=(args.host, args.port),
        interface=args.interface,
        listen_only=args.echo,
    ) as mock:
        try:
            mock.listen(verbose=(args.verbose > 0))
        except KeyboardInterrupt:
            mock.power_off()
