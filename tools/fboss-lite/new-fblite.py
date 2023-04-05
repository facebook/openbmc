#!/usr/local/bin/python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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
# This script auto-generates the machine layer for new BMC-Lite FBOSS
# OpenBMC platforms.
#
# It's done by copying "tools/fboss-lite/fblite-ref-layer" reference layer
# to "meta-facebook/meta-<machine>" and replacing the predefined keywords
# with proper values (such as "@FBMODEL@" --> <machine-name>).
#
# Below are the global configurations in the reference layer, and we need
# to update the reference layer when something is changed:
#   BMC SoC                : AST26xx
#   U-boot Version         : v2019.04
#   Kernel Version         : 6.0.%
#   Yocto Release          : lf-master
#   Init Manager           : systemd
#   Flash data0 Filesystem : UBIFS
#   eMMC filesystem        : EXT4
#   BMC Console            : uart1 (refer to aspeed-g6.dtsi)
#   mTerm Console          : uart5 (refer aspeed-g6.dtsi)
#   BMC MAC Controller     : mac3 (refer aspeed-g6.dtsi)
#   BMC to OOB-Switch      : fixed-link at 1Gbps
#

#
# TODO:
#   2. auto-create Chassis and SCM EEPROM in device tree or setup_i2c.sh.
#   3. auto-generate setup-gpio logic if needed.
#   4. auto-generate "pwrcpld" driver based on its register map.
#   5. auto-generate power control logic if item #4 is automated.
#   6. auto-generate recovery path (depending on BMC-Lite System Reference
#      design)
#

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

#
# Global files and directories.
#
OBMC_BUILD_ENV_FILE = "openbmc-init-build-env"
FBLITE_REF_LAYER = "tools/fboss-lite/fblite-ref-layer"
OBMC_META_FB = "meta-facebook"
CIT_REF_CASES = "tools/fboss-lite/cit-ref-cases"
CIT_RUNNER = "tests2/cit_runner.py"
CIT_TEST_DIR = "tests2/tests"

#
# Predefined keywords in reference layer, and need to be updated when
# producing machine layer code.
#
KEY_MODEL_NAME = "@FBMODEL@"

#
# Global variables
#
TMP_DIR = "/tmp"
COMMIT_TEMPLATE = "tools/fboss-lite/commit-template.txt"
COMMIT_TEMPLATE_CIT = "tools/fboss-lite/commit-template-cit.txt"


def run_shell_cmd(cmd):
    f = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    data, err = f.communicate()
    info = data.decode("utf-8")
    return info


def setup_exec_env():
    """Setup running environment."""
    # Set work directory to the root of openbmc repo
    cli_path = os.path.abspath(sys.argv[0])
    cli_dir = os.path.dirname(cli_path)
    os.chdir(os.path.join(cli_dir, "../.."))
    print("Work directory is set to %s" % os.getcwd())

    # Sanity test if we are at the root of openbmc repo
    for name in [OBMC_BUILD_ENV_FILE, FBLITE_REF_LAYER]:
        if not os.path.exists(name):
            print("Error: %s doesn't exist!" % name)
            sys.exit(1)


def update_machine_code(pathname, name):
    """Replace @KEY@ with proper values in machine layer file."""
    cmd = "sed -i -e 's/%s/%s/g' %s" % (KEY_MODEL_NAME, name, pathname)
    run_shell_cmd(cmd)


def commit_machine_layer(name, machine_layer):
    """Commit the machine layer code to local repo."""
    commit_file = os.path.join(TMP_DIR, "tmp_commit.txt")

    try:
        # Prepare commit message
        shutil.copyfile(COMMIT_TEMPLATE, commit_file)
        cmd = "sed -i -e 's/%s/%s/g' %s" % (KEY_MODEL_NAME, name, commit_file)
        run_shell_cmd(cmd)

        print("Commit the patch to local repo..")
        run_shell_cmd("git add -f %s" % machine_layer)
        run_shell_cmd("git add -f %s" % OBMC_BUILD_ENV_FILE)
        run_shell_cmd("git commit -F %s" % commit_file)
    finally:
        os.remove(commit_file)


def add_new_yocto_version_entry(platname):
    """insert new yocto version entry for new platform."""
    YOCTO_VER = "lf-master"
    SEARCH_STR = ":" + YOCTO_VER
    ANCHOR_STR = "# master rolling-release platforms"
    yct_ents = os.path.join(TMP_DIR, "b_env.txt1")
    other_content = os.path.join(TMP_DIR, "b_env.txt2")
    env_file = OBMC_BUILD_ENV_FILE

    try:
        # extract lf-master platform entries
        cmd = "grep '%s' %s  > %s " % (SEARCH_STR, env_file, yct_ents)
        run_shell_cmd(cmd)
        # add the new entry for new platform
        cmd = "sed -i '1 a \ \ meta-%s%s' %s" % (platname, SEARCH_STR, yct_ents)
        run_shell_cmd(cmd)
        #  sort it to be in order.
        cmd = "sort %s -o %s" % (yct_ents, yct_ents)
        run_shell_cmd(cmd)
        # filter the yocto entries from the openbmc_env file
        cmd = "grep -v '%s' %s  > %s " % (SEARCH_STR, env_file, other_content)
        run_shell_cmd(cmd)
        # merge back
        cmd = "sed -i -e '/%s/r %s' %s" % (ANCHOR_STR, yct_ents, other_content)
        run_shell_cmd(cmd)
        shutil.copyfile(other_content, env_file)
    finally:
        os.remove(yct_ents)
        os.remove(other_content)


def update_cit_code(platname):
    """Add new CIT directory for new model and common CIT cases"""
    cit_plat_path = "%s/%s" % (CIT_TEST_DIR, platname)
    if os.path.exists(cit_plat_path):
        print("Error: %s was already created. Exiting!" % cit_plat_path)
        sys.exit(1)
    shutil.copytree(CIT_REF_CASES, cit_plat_path)

    #
    # Replace @FBMODEL@ with capitalized platform name
    platname_prefix = platname.capitalize()

    for root, _dirs, files in os.walk(cit_plat_path):
        if not files:
            continue

        print("processing CIT scripts under %s.." % root)
        for f_entry in files:
            old_pathname = os.path.join(root, f_entry)
            # remove ".template" sufffix, if exists
            if Path(old_pathname).match("*.template"):
                pathname = os.path.join(root, Path(old_pathname).stem)
                os.rename(old_pathname, pathname)
            else:
                pathname = old_pathname

            cmd = "sed -i -e 's/%s/%s/g' %s" % (
                KEY_MODEL_NAME,
                platname_prefix,
                pathname,
            )
            run_shell_cmd(cmd)


def update_cit_runner(platname):
    """Add new model name to cit_runner.py"""
    CIT_PLATFORM_ANCHOR = "Add new platform name here"
    cmd = "sed -i '/%s/i \\            \"%s\",' %s" % (
        CIT_PLATFORM_ANCHOR,
        platname,
        CIT_RUNNER,
    )
    run_shell_cmd(cmd)
    print("Updated cit_runner")


def commit_cit_changes(name):
    """Commit the cit code to local repo."""
    cit_commit_file = os.path.join(TMP_DIR, "cit_commit.txt")
    cit_folder = os.path.join(CIT_TEST_DIR, "%s" % args.name)

    try:
        # Prepare commit message
        shutil.copyfile(COMMIT_TEMPLATE_CIT, cit_commit_file)
        cmd = "sed -i -e 's/%s/%s/g' %s" % (
            KEY_MODEL_NAME,
            name,
            cit_commit_file,
        )
        run_shell_cmd(cmd)

        print("Commit the CIT patch to local repo..")
        run_shell_cmd("git add -f %s" % cit_folder)
        run_shell_cmd("git add -f %s" % CIT_RUNNER)
        run_shell_cmd("git commit -F %s" % cit_commit_file)
    finally:
        os.remove(cit_commit_file)


if __name__ == "__main__":
    """Create a new fboss-lite machine layer and/or new base CIT test suit"""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-n", "--name", type=str, required=True, help="model name of the new platform"
    )

    parser.add_argument(
        "-p",
        "--purpose",
        type=str,
        required=False,
        help="Create machine layer, and/or base CIT test suit, \
            default is 'machine_layer'",
        choices=[
            "machine_layer",
            "cit",
            "all",
        ],
        default="machine_layer",
    )

    args = parser.parse_args()

    #
    # Set up running environment
    #
    setup_exec_env()

    if args.purpose == "machine_layer" or args.purpose == "all":
        #
        # Copy reference layer to the machine layer
        #
        machine_layer = os.path.join(OBMC_META_FB, "meta-%s" % args.name)
        if os.path.exists(machine_layer):
            print("Error: %s was already created. Exiting!" % machine_layer)
            sys.exit(1)
        print("Copy %s to %s.." % (FBLITE_REF_LAYER, machine_layer))
        shutil.copytree(FBLITE_REF_LAYER, machine_layer)

        #
        # Update files with proper values in the machine layer
        #
        for root, _dirs, files in os.walk(machine_layer):
            if not files:
                continue

            print("processing files under %s.." % root)
            for f_entry in files:
                pathname = os.path.join(root, f_entry)
                update_machine_code(pathname, args.name)

                # Rename leaf files if needed
                if KEY_MODEL_NAME in f_entry:
                    new_name = f_entry.replace(KEY_MODEL_NAME, args.name)
                    os.rename(pathname, os.path.join(root, new_name))

        #
        # update the openbmc-init-build-env file by adding the new yocto distro[] entry
        #
        add_new_yocto_version_entry(args.name)

        #
        # Commit the patch in local tree.
        #
        commit_machine_layer(args.name, machine_layer)

    if args.purpose == "cit" or args.purpose == "all":
        #
        # Update cit_runner.py
        #
        update_cit_runner(args.name)
        #
        # Copy base CIT cases to test2/test/<model_name>
        #
        update_cit_code(args.name)

        print("Added base CIT suit for %s" % (args.name))

        commit_cit_changes(args.name)

        print("Committed base CIT suit for %s " % (args.name))
