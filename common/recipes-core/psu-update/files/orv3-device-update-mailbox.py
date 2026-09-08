#!/usr/bin/env python3
#
# Deprecated wrapper kept for backwards compatibility.
# Use orv3_device_update_mailbox.py instead.

from modbus_impl_pyrmd import Modbus
from modbus_update_helper import auto_int, get_parser
from orv3_device_update_mailbox import main, vendor_params


def parse_args():
    parser = get_parser()
    parser.add_argument(
        "--vendor",
        type=str,
        default="panasonic",
        choices=list(vendor_params.keys()),
        help="Pick vendor for device",
    )
    parser.add_argument("--block-size", type=auto_int, default=None, help="Block Size")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    dev = Modbus(args.addr)
    with dev.suppress_monitoring():
        main(dev, args.file, args.vendor, args.block_size)
