#!/usr/bin/env python3
#
# Deprecated wrapper kept for backwards compatibility.
# Use rpu_update_delta_plc.py instead.

from modbus_impl_pyrmd import Modbus
from modbus_update_helper import get_parser
from rpu_update_delta_plc import main


def parse_args():
    parser = get_parser()
    parser.add_argument(
        "--oem-block",
        action="store_true",
        default=False,
        help="Use OEM Block programming",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    dev = Modbus(args.addr)
    main(dev, args.file, args.oem_block)
