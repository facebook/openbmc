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

import os.path
import re
import json
from subprocess import PIPE, Popen, check_output, CalledProcessError
from uuid import getnode as get_mac

import kv
import pal
from node import node
from vboot import get_vboot_status


# Read all contents of file path specified.
def read_file_contents(path):
    try:
        with open(path, "r") as proc_file:
            content = proc_file.readlines()
    except IOError:
        content = None

    return content


def SPIVendorID2Name(manufacturer_id):
    # Define Manufacturer ID
    MFID_WINBOND = "EF"  # Winbond
    MFID_MICRON = "20"  # Micron
    MFID_MACRONIX = "C2"  # Macronix

    vendor_name = {
        MFID_WINBOND: "Winbond",
        MFID_MICRON: "Micron",
        MFID_MACRONIX: "Macronix",
    }

    if manufacturer_id in vendor_name:
        return vendor_name[manufacturer_id]
    else:
        return "Unknown"


def getSPIVendorLegacy(spi_id):
    cmd = "cat /tmp/spi0.%d_vendor.dat | cut -c1-2" % (spi_id)
    data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
    manufacturer_id = data.strip("\n")
    return SPIVendorID2Name(manufacturer_id)


def getMTD(name):
    mtd_name = '"' + name + '"'
    with open("/proc/mtd") as f:
        lines = f.readlines()
        for line in lines:
            if mtd_name in line:
                return line.split(":")[0]
    return None


def getSPIVendorNew(spi_id):
    mtd = getMTD("flash%d" % (spi_id))
    if mtd is None:
        return "Unknown"
    debugfs_path = "/sys/kernel/debug/mtd/" + mtd + "/partid"
    try:
        with open(debugfs_path) as f:
            data = f.read().strip()
            # Example spi-nor:ef4019
            mfg_id = data.split(":")[-1][0:2].upper()
            return SPIVendorID2Name(mfg_id)
    except Exception:
        pass
    return "Unknown"


def getSPIVendor(spi_id):
    if os.path.isfile("/tmp/spi0.%d_vendor.dat" % (spi_id)):
        return getSPIVendorLegacy(spi_id)
    return getSPIVendorNew(spi_id)


class bmcNode(node):
    def __init__(self, info=None, actions=None):
        if info is None:
            self.info = {}
        else:
            self.info = info
        if actions is None:
            self.actions = []
        else:
            self.actions = actions

    def _getUbootVer(self):
        # Get U-boot Version
        uboot_version = None
        uboot_ver_regex = r"^U-Boot\W+(?P<uboot_ver>20\d{2}\.\d{2})\W+.*$"
        uboot_ver_re = re.compile(uboot_ver_regex)
        mtd_meta = getMTD("meta")
        if mtd_meta == None:
            mtd0_str_dump_cmd = ["/usr/bin/strings", "/dev/mtd0"]
            with Popen(mtd0_str_dump_cmd, stdout=PIPE, universal_newlines=True) as proc:
                for line in proc.stdout:
                    matched = uboot_ver_re.fullmatch(line.strip())
                    if matched:
                        uboot_version = matched.group("uboot_ver")
                        break
        else:
            mtd_dev = "/dev/" + mtd_meta
            with open(mtd_dev, "r") as f:
                raw_data = f.readline()
                try:
                    uboot_version = json.loads(raw_data)["version_infos"]["uboot_ver"]
                except:
                    uboot_version = None
        return uboot_version

    def getUbootVer(self):
        UBOOT_VER_KV_KEY = "u-boot-ver"
        uboot_version = None
        try:
            uboot_version = kv.kv_get(UBOOT_VER_KV_KEY)
        except kv.KeyOperationFailure:
            # not cahced, read and cache it
            uboot_version = self._getUbootVer()
            if uboot_version:
                kv.kv_set(UBOOT_VER_KV_KEY, uboot_version, kv.FCREATE)
        return uboot_version

    def getTpmTcgVer(self):
        out_str = "NA"
        tpm1_caps = "/sys/class/tpm/tpm0/device/caps"
        if os.path.isfile(tpm1_caps):
            with open(tpm1_caps) as f:
                for line in f:
                    if "TCG version:" in line:
                        out_str = line.strip("TCG version: ").strip("\n")
        elif os.path.isfile("/usr/bin/tpm2_getcap"):
            cmd_list = []
            cmd_list.append("/usr/bin/tpm2_getcap -c properties-fixed 2>/dev/null | grep -A2 TPM_PT_FAMILY_INDICATOR")
            cmd_list.append("/usr/bin/tpm2_getcap properties-fixed 2>/dev/null | grep -A2 TPM2_PT_FAMILY_INDICATOR")
            for cmd in cmd_list:
                try:
                    lines = check_output(cmd, shell=True).decode().split("\n")
                    out_str = lines[2].rstrip().split("\"")[1]
                    break
                except Exception as e:
                    pass
        return out_str

    def getTpmFwVer(self):
        out_str = "NA"
        tpm1_caps = "/sys/class/tpm/tpm0/device/caps"
        if os.path.isfile(tpm1_caps):
            with open(tpm1_caps) as f:
                for line in f:
                    if "Firmware version:" in line:
                        out_str = line.strip("Firmware version: ").strip("\n")
        elif os.path.isfile("/usr/bin/tpm2_getcap"):
            cmd_list = []
            cmd_list.append("/usr/bin/tpm2_getcap -c properties-fixed 2>/dev/null | grep TPM_PT_FIRMWARE_VERSION_1")
            cmd_list.append("/usr/bin/tpm2_getcap properties-fixed 2>/dev/null | grep -A1 TPM2_PT_FIRMWARE_VERSION_1 | grep raw")
            for cmd in cmd_list:
                try:
                    line = check_output(cmd, shell=True)
                    value = int(line.decode().rstrip().split(":")[1], 16)
                    out_str = "%d.%d" % (value >> 16, value & 0xFFFF)
                    break
                except Exception as e:
                    pass
        return out_str

    def getMemInfo(self):
        desired_keys = (
            "MemTotal",
            "MemAvailable",
            "MemFree",
            "Shmem",
            "Buffers",
            "Cached",
        )
        meminfo = {}
        with open("/proc/meminfo", "r") as mi:
            for line in mi:
                try:
                    key, value = line.split(":", 1)
                    key = key.strip()
                    if key not in desired_keys:
                        continue
                    memval, _ = value.strip().split(" ", 1)
                    meminfo[key] = int(memval)
                except ValueError:
                    pass
        return meminfo

    def getInformation(self, param=None):
        # Get Platform Name
        name = pal.pal_get_platform_name()

        # Get MAC Address
        eth_intf = pal.pal_get_eth_intf_name()
        mac_path = "/sys/class/net/%s/address" % (eth_intf)
        if os.path.isfile(mac_path):
            mac = open(mac_path).read()
            mac_addr = mac[0:17].upper()
        else:
            mac = get_mac()
            mac_addr = ":".join(("%012X" % mac)[i : i + 2] for i in range(0, 12, 2))

        # Get BMC Reset Reason
        wdt_counter = (
            Popen("devmem 0x1e785010", shell=True, stdout=PIPE).stdout.read().decode()
        )
        wdt_counter = int(wdt_counter, 0)
        wdt_counter &= 0xFF00

        if wdt_counter:
            por_flag = 0
        else:
            por_flag = 1

        if por_flag:
            reset_reason = "Power ON Reset"
        else:
            reset_reason = "User Initiated Reset or WDT Reset"

        # Get BMC's Up Time
        data = Popen("uptime", shell=True, stdout=PIPE).stdout.read().decode()
        uptime = data.strip()

        # Use another method, ala /proc, but keep the old one for backwards
        # compat.
        # See http://man7.org/linux/man-pages/man5/proc.5.html for details
        # on full contents of proc endpoints.
        uptime_seconds = read_file_contents("/proc/uptime")[0].split()[0]

        # Pull load average directory from proc instead of processing it from
        # the contents of uptime command output later.
        load_avg = read_file_contents("/proc/loadavg")[0].split()[0:3]

        # Get Usage information
        data = Popen("top -b n1", shell=True, stdout=PIPE).stdout.read().decode()
        adata = data.split("\n")
        mem_usage = adata[0]
        cpu_usage = adata[1]

        memory = self.getMemInfo()

        # Get OpenBMC version
        obc_version = ""
        data = Popen("cat /etc/issue", shell=True, stdout=PIPE).stdout.read().decode()

        # OpenBMC Version
        ver = re.search(r"[v|V]([\w\d._-]*)\s", data)
        if ver:
            obc_version = ver.group(1)

        # U-Boot version
        uboot_version = self.getUbootVer()
        if uboot_version is None:
            uboot_version = "NA"

        # Get kernel release and kernel version
        kernel_release = ""
        data = Popen("uname -r", shell=True, stdout=PIPE).stdout.read().decode()
        kernel_release = data.strip("\n")

        kernel_version = ""
        data = Popen("uname -v", shell=True, stdout=PIPE).stdout.read().decode()
        kernel_version = data.strip("\n")

        # Get TPM version
        tpm_tcg_version = "NA"
        tpm_fw_version = "NA"
        if os.path.exists("/sys/class/tpm/tpm0"):
            tpm_tcg_version = self.getTpmTcgVer()
            tpm_fw_version = self.getTpmFwVer()

        spi0_vendor = getSPIVendor(0)
        spi1_vendor = getSPIVendor(1)

        # ASD status - check if ASD daemon/asd-test is currently running
        asd_status = bool(
            Popen("ps | grep -i [a]sd", shell=True, stdout=PIPE).stdout.read()
        )
        vboot_info = get_vboot_status()

        used_fd_count = read_file_contents("/proc/sys/fs/file-nr")[0].split()[0]

        info = {
            "Description": name + " BMC",
            "MAC Addr": mac_addr,
            "Reset Reason": reset_reason,
            # Upper case Uptime is for legacy
            # API support
            "Uptime": uptime,
            # Lower case Uptime is for simpler
            # more pass-through proxy
            "uptime": uptime_seconds,
            "Memory Usage": mem_usage,
            "memory": memory,
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
            "load-1": load_avg[0],
            "load-5": load_avg[1],
            "load-15": load_avg[2],
            "open-fds": used_fd_count,
        }

        return info

    def doAction(self, data, param=None):
        Popen("sleep 1; /sbin/reboot", shell=True, stdout=PIPE)
        return {"result": "success"}


def get_node_bmc():
    actions = ["reboot"]
    return bmcNode(actions=actions)
