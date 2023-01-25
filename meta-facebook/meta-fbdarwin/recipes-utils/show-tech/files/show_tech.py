#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

# This script is for dumping debug information on fbdarwin

import argparse
import os
import string
import subprocess
import time


VERSION = "0.10"


def runCmd(cmd, echo=False, verbose=False, timeout=60, ignoreReturncode=False):
    try:
        out = subprocess.Popen(
            cmd.split(" "), stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
    except Exception as e:
        print('"{}" hit exception:\n {}'.format(cmd, e))
        return

    try:
        stdout, stderr = out.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        out.kill()
        print('"{}" timed out in {} seconds'.format(cmd, timeout))
        stdout, stderr = out.communicate()

    output = ""
    if echo:
        output += "{}\n".format(cmd)
    if not ignoreReturncode and out.returncode != 0:
        print('"{}" returned with error code {}'.format(cmd, out.returncode))
        if not verbose:
            return output

    # Ignore non utf-8 characters
    errStr = stderr.decode("ascii", "ignore").encode("ascii").decode()
    outStr = stdout.decode("ascii", "ignore").encode("ascii").decode()

    if stderr:
        output += '"{}" STDERR:\n{}\n'.format(cmd, errStr)

    output += "{}".format(outStr)
    return output


def dumpWeutil(target="BMC", verbose=False):
    cmd = "weutil {}".format(target)
    print(
        "##### {} SERIAL NUMBER #####\n{}".format(
            target, runCmd(cmd, echo=verbose, verbose=verbose)
        )
    )


def dumpEmmc():
    print("################################")
    print("######## eMMC debug log ########")
    print("################################\n")
    cmdBase = "/usr/local/bin/mmcraw {} /dev/mmcblk0"
    print(runCmd(cmdBase.format("show-summary"), echo=True, verbose=True))
    print(runCmd(cmdBase.format("read-cid"), echo=True, verbose=True))


def dumpBootInfo():
    print("################################")
    print("######## BMC BOOT INFO  ########")
    print("################################\n")
    print(runCmd("/usr/local/bin/boot_info.sh bmc", echo=True, verbose=True))
    print(
        "##### FLASH0 META_INFO #####\n{}".format(
            runCmd("/usr/local/bin/meta_info.sh flash0", echo=True, verbose=True)
        )
    )
    print(
        "##### FLASH1 META_INFO #####\n{}".format(
            runCmd("/usr/local/bin/meta_info.sh flash1", echo=True, verbose=True)
        )
    )


def dumpOobStatus():
    print("################################")
    print("####### OOB STATUS INFO  #######")
    print("################################\n")
    print(runCmd("/usr/local/bin/oob-status.sh", echo=True, verbose=True))


def logDump():
    print("################################")
    print("########## DEBUG LOGS ##########")
    print("################################\n")
    print("#### DMESG LOG ####\n{}\n\n".format(runCmd("dmesg", echo=True)))
    print(
        "#### BOOT CONSOLE LOG ####\n{}\n\n".format(
            runCmd("cat /var/log/boot", echo=True)
        )
    )
    print(
        "#### LINUX MESSAGES LOG ####\n{}\n\n".format(
            runCmd("cat /var/log/messages", echo=True)
        )
    )
    print("################################")
    print("########## HOST (uServer) CPU LOGS ##########")
    print("################################\n")
    print(
        "#### mTerm LOG ####\n{}\n{}\n".format(
            runCmd("cat /var/log/mTerm_wedge.log.1", echo=True),
            runCmd("cat /var/log/mTerm_wedge.log", echo=True),
        )
    )


def i2cDetectDump():
    print("################################")
    print("########## I2C DETECT ##########")
    print("################################\n")

    # Only one bus used on FbDarwin BMC.
    bus = 0
    print(
        "##### SMBus{} INFO #####\n{}".format(
            bus,
            runCmd("i2cdetect -y {}".format(bus), verbose=True, timeout=5, echo=True),
        )
    )


def gpioDump():
    print(
        "##### GPIO DUMP #####\n{}".format(
            runCmd("/usr/local/bin/dump_gpios.sh", verbose=True, echo=True)
        )
    )


def showtech(verboseLevel=0):
    verbose = bool(verboseLevel)
    print("################################")
    print("##### SHOWTECH VERSION {} #####".format(VERSION))
    print("################################\n")

    print("##### BMC SYSTEM TIME #####\n{}".format(runCmd("date")))
    print("##### BMC HOSTNAME #####\n{}".format(runCmd("hostname")))
    print(
        "##### BMC version #####\nbuilt: {}{}".format(
            runCmd("cat /etc/version"), runCmd("cat /etc/issue")
        )
    )
    print(
        "##### BMC Board Revision #####\n{}".format(
            runCmd("/usr/local/bin/bmc_board_rev.sh")
        )
    )
    print("##### BMC UPTIME #####\n{}".format(runCmd("uptime")))

    if verbose:
        dumpWeutil("BMC", verbose=verbose)
        dumpEmmc()
        dumpBootInfo()
        dumpOobStatus()
        i2cDetectDump()
        gpioDump()
        logDump()


def parseArgs():
    parser = argparse.ArgumentParser(
        description="Get showtech information. Version {}".format(VERSION)
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        help="show verbose detailed debug logs.",
        dest="verboseLevel",
        default=0,
    )
    return parser.parse_args()


def main():
    args = parseArgs()

    showtech(verboseLevel=args.verboseLevel)


if __name__ == "__main__":
    main()
