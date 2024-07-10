#!/usr/bin/python3

import argparse
import csv
import json

description = """
Convert Register Map between JSON and CSV

Helper script to convert a register map specified in JSON
(format used as input by rackmon) or CSV (after dropping
device specific information) and vice-versa.

JSON -> CSV is primarily useful to export the JSON to things like
wiki or other documentation sources meant for human consumption.

CSV -> JSON: The script puts stubs/dummy-values for non-register map
values like register-map name, probe_register etc. The caller
is expected to fill those. This allows the CSV to be a simple
flat-map of just the registers without any device descriptors.
"""

CSV_HEADERS = {
    "begin": int,
    "length": int,
    "keep": int,
    "changes_only": bool,
    "flags_bit": int,
    "flags_name": str,
    "format": str,
    "endian": str,
    "interval": int,
    "precision": int,
    "sign": bool,
    "scale": float,
    "shift": float,
    "name": str,
}
CSV_MANDATORY_HEADERS = {"begin", "length", "flags_bit", "flags_name", "format", "name"}


def reg_convert(reg):
    """
    Return a reg dict (as read from CSV) after performing
    type conversions (converts "123" to 123).
    Also validates input to make sure it works. So if user
    passes in blah instead of a expected number, it will throw.
    """
    out = {}
    for header in CSV_HEADERS:
        if header in reg and reg[header] != "":
            if CSV_HEADERS[header] is bool:
                out[header] = reg[header].capitalize() == "True"
            else:
                out[header] = CSV_HEADERS[header](reg[header])
    return out


def validateRow(row, isFlag):
    """
    Validates if the CSV row contains expected/validated values
    """
    try:
        if isFlag:
            assert int(row["flags_bit"]) >= 0
            assert isinstance(row["flags_name"], str) and row["flags_name"] != ""
        else:
            assert (
                row["begin"] != ""
                and int(row["begin"]) >= 0
                and int(row["begin"]) <= 0xFFFF
            )
            assert (
                row["length"] != ""
                and int(row["length"]) >= 0
                and int(row["length"]) <= 0xFFFF
            )
            assert row["keep"] == "" or int(row["keep"]) > 0
            assert row["changes_only"] == "" or row["changes_only"].capitalize() in [
                "True",
                "False",
            ]
            assert row["sign"] == "" or row["sign"].capitalize() in ["True", "False"]
            assert row["format"] in ["STRING", "INTEGER", "FLOAT", "FLAGS"]
            assert row["endian"] == "" or row["endian"] in row["endian"] in ["L", "B"]
            assert row["interval"] == "" or int(row["interval"]) > -2
            assert row["format"] != "FLOAT" or int(row["precision"]) >= 0
            assert (
                row["format"] != "FLOAT"
                or row["scale"] == ""
                or float(row["scale"]) > 0.0
            )
            assert (
                row["format"] != "FLOAT"
                or row["shift"] == ""
                or float(row["shift"]) != 0.0
            )
            assert row["name"] != ""

    except Exception:
        print(f"Row failed validation: {json.dumps(row)}")
        raise


def stubRow(row):
    """
    Adds missing keys with a default "" to the CSV row.
    """
    for h in CSV_HEADERS:
        if h not in row:
            row[h] = ""


def readRegs(csvfile):
    """
    Reads registers from CSV file and returns a dict which can be interpreted by
    rackmon as the value for "registers".
    """
    reader = csv.DictReader(csvfile)
    readHeaders = set(reader.fieldnames)
    if not CSV_MANDATORY_HEADERS.issubset(readHeaders):
        raise ValueError(
            f"Input CSV does not contain mandatory headers: {CSV_MANDATORY_HEADERS}"
        )
    if not readHeaders.issubset(set(CSV_HEADERS.keys())):
        additional = readHeaders - set(CSV_HEADERS.keys())
        print(f"WARNING: Input CSV contains unrecognized columns: {additional}")
    regs = []
    lastFlagReg = None
    for row in reader:
        stubRow(row)
        if row["format"] != "" and lastFlagReg is not None:
            # Check if we are done with flags and append them.
            regs.append(lastFlagReg)
            lastFlagReg = None
        validateRow(row, lastFlagReg is not None)
        if row["format"] == "FLAGS":
            lastFlagReg = reg_convert(row)
            name = lastFlagReg["name"]
            del lastFlagReg["name"]
            lastFlagReg["flags"] = []
            lastFlagReg["name"] = name
        elif lastFlagReg is not None:
            lastFlagReg["flags"].append([int(row["flags_bit"]), row["flags_name"]])
        else:
            regs.append(reg_convert(row))
    if lastFlagReg is not None:
        regs.append(lastFlagReg)
    return regs


def dump_reg(reg, writer):
    """
    Writes a register to a CSV as a row.
    """
    if reg["format"] == "FLAGS":
        flags = reg["flags"]
        del reg["flags"]
        writer.writerow(reg)
        for flag in flags:
            out = {"flags_bit": flag[0], "flags_name": flag[1]}
            writer.writerow(out)
    else:
        writer.writerow(reg)


def dump_csv(regs, csvfile):
    """
    Writes all reigsters as rows a specified CSV file.
    """
    writer = csv.DictWriter(csvfile, fieldnames=CSV_HEADERS.keys())
    writer.writeheader()
    for reg in regs:
        dump_reg(reg, writer)


def parse_args():
    """
    Sets up arguments for the scripts, parses it and returns
    the current values.
    """
    parser = argparse.ArgumentParser(
        description=description,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "input",
        help="Input register map file (JSON or CSV)",
    )
    parser.add_argument(
        "-o", "--output", default=None, help="Output file (JSON or CSV based on input)"
    )
    args = parser.parse_args()
    if args.output is None:
        if args.input.lower().endswith(".json"):
            args.output = "a.csv"
        else:
            args.output = "a.json"
    return args


def main():
    """
    Main function
    """
    args = parse_args()
    if args.input.lower().endswith(".json"):
        with open(args.input) as f:
            data = json.load(f)
        with open(args.output, "w", newline="") as csvfile:
            dump_csv(data["registers"], csvfile)
    else:
        with open(args.input) as csvfile:
            out = {
                "name": "TODO",
                "address_range": [[0, 0]],
                "probe_register": 0,
                "default_baudrate": 19200,
                "preferred_baudrate": 19200,
                "registers": readRegs(csvfile),
            }
            with open(args.output, "w") as f:
                json.dump(out, f, indent=4)


main()
