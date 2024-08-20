#!/usr/bin/env python3
import json
import sys


def check_addr(addr, ranges):
    for arange in ranges:
        if addr >= arange[0] and addr <= arange[1]:
            return True
    return False


def check_range(ranges, arange):
    addrs = list(range(arange[0], arange[1] + 1))
    for addr in addrs:
        if check_addr(addr, ranges):
            return True
    return False


def main(name, files):
    if len(files) < 1:
        print("usage: {} jsons".format(name))
        sys.exit(1)
    address_ranges = []
    for conf in files:
        with open(conf) as f:
            data = json.load(f)
            aranges = data["address_range"]
            for arange in aranges:
                if check_range(address_ranges, arange):
                    print("failure:", conf, file=sys.stderr)
                    sys.exit(1)
                address_ranges.append(arange)
            print("success: ", conf)


if __name__ == "__main__":
    main(sys.argv[0], sys.argv[1:])
