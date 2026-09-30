#!/usr/bin/env python3
#
# Deprecated wrapper kept for backwards compatibility.
# Use rpu_update_coolermaster.py instead.

from modbus_impl_pyrmd import Modbus
from modbus_update_helper import get_parser
from rpu_update_coolermaster import AALCV2_COMPONENTS, main


def parse_args():
    parser = get_parser()
    parser.add_argument(
        "--component",
        type=str,
        default=None,
        choices=list(AALCV2_COMPONENTS.keys()),
        help="Component to update",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    dev = Modbus(args.addr)
    with dev.suppress_monitoring():
        main(dev, args.file, args.component)
