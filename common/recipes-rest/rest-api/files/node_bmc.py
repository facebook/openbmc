#!/usr/bin/env python
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

from subprocess import *
import re
from node import node
from pal import *
from uuid import getnode as get_mac
import os.path
from vboot import get_vboot_status

def getSPIVendor(manufacturer_id):
    # Define Manufacturer ID
    MFID_WINBOND = "EF"    # Winbond
    MFID_MICRON = "20"     # Micron
    MFID_MACRONIX = "C2"   # Macronix

    vendor_name = {
        MFID_WINBOND: "Winbond",
        MFID_MICRON: "Micron",
        MFID_MACRONIX: "Macronix",
        }

    if manufacturer_id in vendor_name:
        return(vendor_name[manufacturer_id])
    else:
        return("Unknown")

class bmcNode(node):
    def __init__(self, info = None, actions = None):
        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self, param={}):
        # Get Platform Name
        name = pal_get_platform_name().decode()

        # Get MAC Address
        mac_path = "/sys/class/net/eth0/address"
        if os.path.isfile(mac_path):
            mac = open('/sys/class/net/eth0/address').read()
            mac_addr = mac[0:17].upper()
        else:
            mac = get_mac()
            mac_addr = ':'.join(("%012X" % mac)[i:i+2] for i in range(0, 12, 2))

        # Get BMC Reset Reason
        wdt_counter = Popen('devmem 0x1e785010', \
                            shell=True, stdout=PIPE).stdout.read().decode()
        wdt_counter = int(wdt_counter, 0)
        wdt_counter &= 0xff00

        if wdt_counter:
            por_flag = 0
        else:
            por_flag = 1

        if por_flag:
            reset_reason = "Power ON Reset"
        else:
            reset_reason = "User Initiated Reset or WDT Reset"

        # Get BMC's Up Time
        data = Popen('uptime', \
                        shell=True, stdout=PIPE).stdout.read().decode()
        uptime = data.strip()

        # Get Usage information
        data = Popen('top -b n1', \
                            shell=True, stdout=PIPE).stdout.read().decode()
        adata = data.split('\n')
        mem_usage = adata[0]
        cpu_usage = adata[1]

        # Get OpenBMC version
        obc_version = ""
        data = Popen('cat /etc/issue', \
                            shell=True, stdout=PIPE).stdout.read().decode()

        # OpenBMC Version
        ver = re.search(r'[v|V]([\w\d._-]*)\s', data)
        if ver:
            obc_version = ver.group(1)

        # Get U-boot Version
        uboot_version = ""
        data = Popen( 'strings /dev/mtd0 | grep U-Boot | grep 20', \
                            shell=True, stdout=PIPE).stdout.read().decode()

        # U-boot Version
        lines=data.splitlines()
        data_len=len(lines)
        for i in range(data_len):
            if i!=data_len-1:
                uboot_version += lines[i] +", "
            else:
                uboot_version += lines[i]

        # Get kernel release and kernel version
        kernel_release = ""
        data = Popen( 'uname -r', \
                            shell=True, stdout=PIPE).stdout.read().decode()
        kernel_release = data.strip('\n')

        kernel_version = ""
        data = Popen( 'uname -v', \
                            shell=True, stdout=PIPE).stdout.read().decode()
        kernel_version = data.strip('\n')

        # Get TPM version
        tpm_path = "/sys/class/tpm/tpm0/device/caps"
        if os.path.isfile(tpm_path):
            with open(tpm_path) as f:
                for line in f:
                    if 'TCG version:' in line:
                        tpm_tcg_version = line.strip("TCG version: ").strip('\n')
                    elif 'Firmware version:' in line:
                        tpm_fw_version = line.strip("Firmware version: ").strip('\n')
        else:
            tpm_tcg_version = "NA"
            tpm_fw_version = "NA"

        # SPI0 Vendor
        spi0_vendor = ""
        data = Popen('cat /tmp/spi0.0_vendor.dat | cut -c1-2', \
                            shell=True, stdout=PIPE).stdout.read().decode()
        spi0_mfid = data.strip('\n')
        spi0_vendor = getSPIVendor(spi0_mfid)

        # SPI1 Vendor
        spi1_vendor = ""
        data = Popen('cat /tmp/spi0.1_vendor.dat | cut -c1-2', \
                            shell=True, stdout=PIPE).stdout.read().decode()
        spi1_mfid = data.strip('\n')
        spi1_vendor = getSPIVendor(spi1_mfid)

        #ASD status - check if ASD daemon/asd-test is currently running
        asd_status = bool(Popen('ps | grep -i [a]sd', shell=True, stdout=PIPE).stdout.read())
        vboot_info = get_vboot_status()

        info = {
            "Description": name + " BMC",
            "MAC Addr": mac_addr,
            "Reset Reason": reset_reason,
            "Uptime": uptime,
            "Memory Usage": mem_usage,
            "CPU Usage": cpu_usage,
            "OpenBMC Version": obc_version,
            "u-boot version": uboot_version,
            "kernel version": kernel_release + " " + kernel_version,
            "TPM TCG version": tpm_tcg_version,
            "TPM FW version": tpm_fw_version,
            "SPI0 Vendor": spi0_vendor,
            "SPI1 Vendor": spi1_vendor,
            "At-Scale-Debug Running": asd_status,
            "vboot": vboot_info,
            }

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if (data["action"] != 'reboot'):
                result = 'failure'
            else:
                Popen('sleep 1; /sbin/reboot', shell=True, stdout=PIPE)
                result = 'success'

            result = {"result": result}

        return result

def get_node_bmc(is_read_only=True):
    if is_read_only:
        actions =  []
    else:
        actions = ["reboot"]
    return bmcNode(actions = actions)
