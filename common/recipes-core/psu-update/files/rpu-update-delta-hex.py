#!/usr/bin/env python3
#
# Deprecated wrapper kept for backwards compatibility.
# Use rpu_update_delta_hex.py instead.

from modbus_impl_pyrmd import Modbus
from modbus_update_helper import get_parser
from rpu_update_delta_hex import main


def parse_args():
    return get_parser().parse_args()


if __name__ == "__main__":
    args = parse_args()
    dev = Modbus(args.addr)
    main(dev, args.file)
