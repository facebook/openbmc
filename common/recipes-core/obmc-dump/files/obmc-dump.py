#!/usr/bin/env python3
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
import argparse
import glob
import os
import pathlib
import shutil
import subprocess
import time
import traceback

# Ensure at least this much free space is always present.
PERSISTENT_STORE_HEADROOM = 1024 * 100
DUMP_DIR = "/tmp/obmc-dump-%d" % (int(time.time()))

DUMP_PATHS = ["/var/log", "/tmp/cache_store", "/mnt/data/kv_store"]
DUMP_COMMANDS = [["fw-util", "all", "--version"], ["ifconfig"]]


def copy(path):
    print("COPY: " + path)
    dest = DUMP_DIR + path
    pathlib.Path(os.path.dirname(dest)).mkdir(parents=True, exist_ok=True)
    if os.path.isdir(path):
        shutil.copytree(path, dest)
    else:
        shutil.copy(path, dest)


def command_output(cmd):
    print("COMMAND: ", " ".join(cmd))
    out = subprocess.check_output(cmd).decode()
    cmd_file = DUMP_DIR + "/" + os.path.basename(cmd[0]) + ".txt"
    with open(cmd_file, "a+") as f:
        f.write("=" * 32 + "\n")
        f.write("COMMAND: " + " ".join(cmd) + "\n")
        f.write("-" * 32 + "\n")
        f.write(out + "\n")
        f.write("=" * 32 + "\n")


def get_persistent_stores():
    out = subprocess.check_output(["df"]).decode().splitlines()
    hdr = out.pop(0).split()
    parts = [dict(zip(hdr, row.split())) for row in out]
    ret = []
    for part in parts:
        mount = part["Mounted"]
        avail = int(part["Available"])
        if mount.startswith("/mnt/data"):
            ret.append([mount, avail * 1024])
    return ret


def cleanup_persistent_stores(stores, def_yes=False):
    for store in stores:
        path = store[0]
        dumps = glob.glob(path + "/obmc-dump*.tar.gz")
        for f in dumps:
            if (
                def_yes is True
                or input("Remove " + f + " (y/n): ").lower().strip() == "y"
            ):
                print("Removing: " + f)
                os.remove(f)


def obmc_dump_cleanup(def_yes=False):
    persist_stores = get_persistent_stores()
    cleanup_persistent_stores(persist_stores, def_yes)


def obmc_dump_persist(archive):
    persist_stores = get_persistent_stores()
    store = None
    store_size = 0
    for store in persist_stores:
        p = store[0]
        size = store[1]
        if size > store_size:
            store = p
            store_size = size
    sz = os.path.getsize(archive)
    if sz > (store_size - PERSISTENT_STORE_HEADROOM):
        print("Not enough space in persistent stores")
        print("Archive size:", sz)
        print("Store available size: ", store_size)
        return archive
    shutil.move(archive, store + "/")
    return store + "/" + os.path.basename(archive)


def obmc_dump():
    os.mkdir(DUMP_DIR)
    errors = ""
    for p in DUMP_PATHS:
        if os.path.exists(p):
            copy(p)
    for c in DUMP_COMMANDS:
        try:
            command_output(c)
        except Exception:
            errors += traceback.format_exc()

    with open(DUMP_DIR + "/obmc_dump_errors.txt", "w") as f:
        f.write(errors + "\n")

    name = os.path.basename(DUMP_DIR)
    subprocess.check_call(["tar", "-czf", name + ".tar.gz", name], cwd="/tmp")
    shutil.rmtree(DUMP_DIR)
    archive = DUMP_DIR + ".tar.gz"
    archive = obmc_dump_persist(archive)
    sz = os.path.getsize(archive)
    print("Generated:", archive, "of size", sz)
    return archive


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="OpenBMC Dump Tool")
    parser.add_argument(
        "-y", "--yes", dest="yes", action="store_true", help="Answer Yes to all prompts"
    )
    args = parser.parse_args()
    obmc_dump_cleanup(args.yes)
    obmc_dump()
