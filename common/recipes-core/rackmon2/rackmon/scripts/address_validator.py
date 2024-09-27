#!/usr/bin/env python3
import json
import sys


def check_addr(addr, baudrate, ranges):
    if ranges[addr] is not None and baudrate in ranges[addr]["baudrates"]:
        return True
    return False


def check_range(ranges, arange, baudrate, name):
    addrs = list(range(arange[0], arange[1] + 1))
    for addr in addrs:
        if check_addr(addr, baudrate, ranges):
            print(f"Conflict found for {addr} adding {name}:", ranges[addr])
            return True
        if ranges[addr] is None:
            ranges[addr] = {"names": [name], "baudrates": [baudrate]}
        else:
            ranges[addr]["baudrates"].append(baudrate)
            ranges[addr]["names"].append(name)
    return False


def main(name, files):
    if len(files) < 1:
        print("usage: {} jsons".format(name))
        sys.exit(1)
    address_ranges = dict(zip(list(range(0, 256)), [None] * 256))
    for conf in files:
        with open(conf) as f:
            data = json.load(f)
            baudrate = data["baudrate"]
            aranges = data["address_range"]
            name = data["name"]
            for arange in aranges:
                if check_range(address_ranges, arange, baudrate, name):
                    print("failure:", conf, file=sys.stderr)
                    sys.exit(1)
            print("success: ", conf)


if __name__ == "__main__":
    main(sys.argv[0], sys.argv[1:])
