#!/usr/bin/python3
import argparse
import ctypes
import json
import re
import subprocess
import sys

import pal
import sdr


def cmd(c):
    try:
        return subprocess.check_output(c).decode()
    except subprocess.CalledProcessError as ex:
        return ex.output.decode()


def frunames():
    out = cmd(["sensor-util"])
    rx = re.compile(R"\[fru\]\s+:\s+(.+)$")
    for line in out.splitlines():
        m = rx.search(line)
        if m:
            return [s.strip() for s in m.group(1).split(", ")]


def get_name2idx():
    out = cmd(["sensor-util", "all"]).splitlines()
    rx = re.compile(R"(.+)\s+\(0x([a-fA-F\d]+)\) :")
    ret = {}
    for line in out:
        line = line.strip()
        if line == "":
            continue
        m = rx.search(line)
        if not m:
            continue
        sname, sid = m.group(1).strip(), "0x" + m.group(2)
        if sname in ret:
            if ret[sname] != sid:
                print(
                    sname,
                    " exists with id:",
                    ret[sname],
                    " but new sensor same name with id: ",
                    sid,
                )
            continue
        ret[sname] = sid
    return ret


def get_units_map():
    out = cmd(["sensor-util", "all"]).splitlines()
    rx = re.compile(R"(.+)\s+\(0x.+\s+(\S+)\s+\|\s+\(ok\)$")
    ret = {}
    for line in out:
        line = line.strip()
        m = rx.search(line)
        if m:
            ret[m.group(1).strip()] = m.group(2)
    return ret


def fru_fullname_map():
    libpal = ctypes.CDLL("libpal.so.0")
    c_fru_name = ctypes.create_string_buffer(64)
    return {
        n: (n, i)
        if libpal.pal_get_fruid_name(i, c_fru_name) != 0
        else (c_fru_name.value.decode(), i)
        for n, i in pal.pal_fru_name_map().items()
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="OpenBMC Sensor Spec Generator")
    parser.add_argument("platform", help="Full Name of the Platform")
    parser.add_argument(
        "-f", dest="file", default=None, help="Optional file path to store spec"
    )
    args = parser.parse_args()
    full_name_map = fru_fullname_map()
    names = frunames()
    if names[0] == "all":
        names = names[1:]
    if args.file is not None:
        sys.stdout = open(args.file, "w")
    print(f"# Facebook OpenBMC Sensor List for {args.platform}")
    print("")
    print(f"This document of sensor lists for platform {args.platform}.")
    print("")
    sys.stdout.flush()
    name2id = get_name2idx()
    units_map = get_units_map()
    for name in names:
        if name not in full_name_map:
            continue
        full_name = full_name_map[name][0]
        fru_id = full_name_map[name][1]
        print(f"## {full_name}")
        print("| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |")
        print("| ----------- | ------- | ---- | --- | --- | --- | --- | --- | --- |")
        sys.stdout.flush()
        datastr = cmd(["sensor-util", name, "--threshold", "--json"])
        # Workaround for bug fixed by D35586808
        if datastr.startswith(f"{name}:"):
            datastr = datastr.replace(f"{name}:", "", 1).strip()
        data = json.loads(datastr)
        keytoidx = {"UNR": 2, "UCR": 3, "UNC": 4, "LNC": 5, "LCR": 6, "LNR": 7}
        for snr in data:
            sdata = data[snr]
            if snr not in units_map:
                units_map[snr] = sdr.sdr_get_sensor_units(fru_id, int(name2id[snr], 16))
            row = [
                snr,
                name2id[snr],
                units_map[snr],
                "NA",
                "NA",
                "NA",
                "NA",
                "NA",
                "NA",
            ]
            if "thresholds" in sdata:
                ther = sdata["thresholds"]
                for k, v in ther.items():
                    idx = keytoidx[k]
                    row[idx] = v
            # JSON data does not contain information on units and sensor ID. :-/
            print("|", " | ".join(row), "|")
            sys.stdout.flush()
        print("")
        print("")
