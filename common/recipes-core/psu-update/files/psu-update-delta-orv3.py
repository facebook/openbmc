#!/usr/bin/env python3
#
# Deprecated wrapper kept for backwards compatibility.
# Use psu_update_delta_orv3.py instead.

from modbus_impl_pyrmd import Modbus
from modbus_update_helper import auto_int, get_parser
from psu_update_delta_orv3 import main


def parse_args():
    parser = get_parser()
    parser.add_argument("--key", type=auto_int, default=None, help="Sec key")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    dev = Modbus(args.addr)
    with dev.suppress_monitoring():
        main(dev, args.file, args.key)
