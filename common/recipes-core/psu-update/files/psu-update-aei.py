#!/usr/bin/env python3
#
# Deprecated wrapper kept for backwards compatibility.
# Use psu_update_aei.py instead.

from modbus_impl_pyrmd import Modbus
from modbus_update_helper import get_parser
from psu_update_aei import device_params, main


def parse_args():
    parser = get_parser()
    parser.add_argument(
        "--device",
        type=str,
        default="orv3",
        choices=list(device_params.keys()),
        help="Pick device type",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    dev = Modbus(args.addr)
    main(dev, args.file, args.device)
