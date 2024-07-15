#!/usr/bin/python3

import argparse
import json
import subprocess
import sys


def curl_get(path, server="rsw1cj-oob.01.snc8.facebook.com"):
    out = subprocess.check_output(
        ["curl", "-ks", f"https://{server}:8443{path}"]
    ).decode()
    return out


def get_raw(server):
    return json.loads(json.loads(curl_get("/api/sys/modbus_registers")))


def get_data(server):
    return json.loads(curl_get("/api/sys/modbus/registers"))


def merge(values, raw):
    merged = {}
    for d in values:
        info = d["devInfo"]
        addr = info["devAddress"]
        regs = d["regList"]
        merged[addr] = {}
        for reg in regs:
            history = reg["history"]
            name = reg["name"]
            raddr = reg["regAddress"]
            merged[addr][raddr] = {"name": name, "type": None, "history": {}}
            for value in history:
                vtype = value["type"]
                if merged[addr][raddr]["type"] is None:
                    merged[addr][raddr]["type"] = vtype
                elif merged[addr][raddr]["type"] != vtype:
                    pname = f"{addr}:{name}:{raddr}"
                    existing = merged[addr][raddr]["type"]
                    print(f"ERROR: Ignoring {pname} type {existing}->{vtype}")
                tsp = "%d" % value["timestamp"]
                val = list(value["value"].values())[0]
                merged[addr][raddr]["history"][tsp] = {"value": val}
    for d in raw:
        addr = d["addr"]
        regs = d["ranges"]
        if addr not in merged:
            print(f"Ignoring dev={addr} in raw but not in values")
            continue
        for reg in regs:
            raddr = reg["begin"]
            if raddr not in merged[addr]:
                print(f"Ignoring dev={addr} reg={raddr} in raw but not in values")
                continue
            history = reg["readings"]
            for rvalue in history:
                r = rvalue["data"]
                tsp = "%d" % rvalue["time"]
                if tsp not in merged[addr][raddr]["history"]:
                    print(f"Did not find {tsp} in {merged[addr][raddr]['history']}")
                    continue
                merged[addr][raddr]["history"][tsp]["raw"] = r
    return merged


def print_merged(merged, f):
    headers = [
        "Device Address",
        "Register Name",
        "Register Address",
        "Timestamp",
        "Hex Raw Value",
        "Parsed Value",
    ]
    print(",".join(headers), file=f)
    for addr, dev in merged.items():
        for raddr, reg in dev.items():
            name = reg["name"]
            rtype = reg["type"]
            for tsp, val in reg["history"].items():
                if "raw" not in val:
                    raw = "UNKNOWN"
                else:
                    raw = val["raw"]
                tsp = int(tsp)
                print(
                    f"0x{addr:02x},{name},0x{raddr:04x},0x{tsp:08x},0x{raw}",
                    end="",
                    file=f,
                )
                if "value" not in val:
                    print(",UNKNOWN", file=f)
                    continue
                value = val["value"]
                if rtype == "FLAGS":
                    for b in value:
                        bname = "%s<%d>" % (b["name"], b["bitOffset"])
                        bval = "1" if b["value"] is True else "0"
                        print(f",{bname}={bval}", end="", file=f)
                else:
                    print(f",{value}", end="", file=f)
                print("", file=f)


def rackmon_action(server, action):
    cmd = ["rackmoncli", action]
    if server != "localhost":
        cmd = ["ssh", f"root@{server}"] + cmd
    subprocess.check_call(cmd)


def main(bmc, output):
    rackmon_action(bmc, "pause")
    values = get_data(bmc)
    raw = get_raw(bmc)
    rackmon_action(bmc, "resume")
    print_merged(merge(values, raw), output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Dump rackmon register data as CSV",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "bmc",
        help="Hostname of the BMC",
        nargs="?",
        default="localhost",
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Output file name",
        action="store",
        type=argparse.FileType("w"),
        dest="output",
        default=sys.stdout,
    )
    args = parser.parse_args()

    main(args.bmc, args.output)
