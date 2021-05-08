#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

# This script is for dumping debug information on yamp

import argparse
import subprocess
import time


VERSION = "0.2"
SC_POWERGOOD = "/sys/bus/i2c/drivers/supcpld/12-0043/switchcard_powergood"


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


def dumpWeutil(target="CHASSIS", verbose=False):
    cmd = "weutil {}".format(target)
    print(
        "##### {} SERIAL NUMBER #####\n{}".format(
            target, runCmd(cmd, echo=verbose, verbose=verbose)
        )
    )


def fan_debuginfo(verbose=False):
    print("################################")
    print("######## FAN DEBUG INFO ########")
    print("################################\n")
    print(runCmd("cat /sys/bus/i2c/devices/13-0060/fan_card_overtemp", echo=True))
    for _ in range(1, 6):
        fanPrefix = "/sys/bus/i2c/devices/13-0060/fan{}".format(_)
        log = "##### FAN {} DEBUG INFO #####\n".format(_)
        log += runCmd("head -n 1 {}_pwm".format(fanPrefix), echo=True)
        log += runCmd("head -n 1 {}_present".format(fanPrefix), echo=True)
        log += runCmd("head -n 1 {}_id".format(fanPrefix), echo=True)
        log += runCmd("head -n 1 {}_present_change".format(fanPrefix), echo=True)
        log += "### FAN {} LED INFO ### 0x0 - ON, 0x1 - OFF\n".format(_)
        log += runCmd("head -n 1 {}_led_red".format(fanPrefix), echo=True)
        log += runCmd("head -n 1 {}_led_green".format(fanPrefix), echo=True)
        print(log)

    print("##### FAN SPEED LOGS #####")
    for _ in range(2):
        print(runCmd("/usr/local/bin/get_fan_speed.sh", echo=True))
        print("sleeping 0.5 seconds...")
        time.sleep(0.5)

    if verbose:
        print("##### FAN CPLD I2CDUMP #####\n")
        print(runCmd("i2cdump -f -y 13 0x60", echo=True))


def switchcard_debuginfo():
    print("################################")
    print("##### SWITCHCARD DEBUG INFO ####")
    print("################################\n")
    print("##### SMB CPLD I2CDUMP #####\n")
    print(runCmd("i2cdump -f -y 4 0x23", echo=True))


def pim_debuginfo():
    print("################################")
    print("######## PIM DEBUG INFO ########")
    print("################################\n")
    print("##### SCM CPLD I2CDUMP #####\n")
    print(runCmd("i2cdump -f -y 12 0x43", echo=True))


def psu_debuginfo():
    print("################################")
    print("######## PSU DEBUG INFO ########")
    print("################################\n")
    for i in range(1, 4 + 1):
        # PSU SMBus range 24-28 for PSU1-4
        # YAMPTODO change from generic
        cmd = "/usr/local/bin/psu_show_tech.py {} 0x58 -c generic".format(23 + i)
        print(
            "##### PSU{} INFO #####\n{}".format(i, runCmd(cmd, echo=True, verbose=True))
        )


def logDump():
    print("################################")
    print("########## DEBUG LOGS ##########")
    print("################################\n")
    print("#### SENSORS LOG ####\n{}\n\n".format(runCmd("sensors", echo=True)))
    print(
        "#### FSCD LOG ####\n{}\n{}\n".format(
            runCmd("cat /var/log/fscd.log.1", echo=True),
            runCmd("cat /var/log/fscd.log", echo=True),
        )
    )
    print("#### DMESG LOG ####\n{}\n\n".format(runCmd("dmesg", echo=True)))
    print(
        "#### BOOT CONSOLE LOG ####\n{}\n\n".format(
            runCmd("cat /var/log/boot", echo=True)
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
    print("################################")
    print("##########  DPM LOGS  ##########")
    print("################################\n")
    print(
        "#### DPM LOG ####\n{}\n".format(
            runCmd("cat /mnt/data1/log/dpm_log", echo=True),
        )
    )


def i2cDetectDump():
    print("################################")
    print("########## I2C DETECT ##########")
    print("################################\n")
    for bus in range(0, 22):
        print(
            "##### SMBus{} INFO #####\n{}".format(
                bus,
                runCmd(
                    "i2cdetect -y {}".format(bus), verbose=True, timeout=5, echo=True
                ),
            )
        )


def showtech(verbose=False):
    print("################################")
    print("##### SHOWTECH VERSION {} #####".format(VERSION))
    print("################################\n")

    print(
        "##### USER PWR STATUS #####\n{}".format(
            runCmd("/usr/local/bin/wedge_power.sh status")
        )
    )
    print(
        "##### SWITCHCARD POWERGOOD STATUS #####\n{}".format(
            runCmd("head -n 1 {}".format(SC_POWERGOOD))
        )
    )
    print("##### BMC SYSTEM TIME #####\n{}".format(runCmd("date")))
    print("##### BMC version #####\n{}".format(runCmd("cat /etc/issue")))
    print("##### BMC UPTIME #####\n{}".format(runCmd("uptime")))
    print(
        "##### FPGA VERSIONS #####\n{}".format(
            runCmd("/usr/local/bin/fpga_ver.sh", verbose=True)
        )
    )
    print(
        "##### PIM TYPES #####\n{}".format(
            runCmd("/usr/local/bin/pim_types.sh", verbose=True)
        )
    )

    # Dump EEPROMS
    dumpWeutil("CHASSIS", verbose=verbose)
    dumpWeutil("SCD", verbose=verbose)
    dumpWeutil("SUP", verbose=verbose)

    # Go through PIM 1-8
    for pim in range(1, 8 + 1):
        dumpWeutil("lc{}".format(pim), verbose=verbose)
    if verbose:
        print(
            "##### DPM VERSIONS #####\n{}".format(
                runCmd("/usr/local/bin/dpm_ver.sh", verbose=verbose)
            )
        )
        fan_debuginfo(verbose=verbose)
        if verbose:
           switchcard_debuginfo()
           pim_debuginfo()
           i2cDetectDump()
        logDump()


def parseArgs():
    parser = argparse.ArgumentParser(
        description="Get showtech information. Version {}".format(VERSION)
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="show verbose detailed debug logs."
    )
    return parser.parse_args()


def main():
    args = parseArgs()

    showtech(verbose=args.verbose)


if __name__ == "__main__":
    main()
