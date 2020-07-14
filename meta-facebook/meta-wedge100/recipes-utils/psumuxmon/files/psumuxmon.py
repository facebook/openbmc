#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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
import os
import re
import subprocess
import syslog
import time
from typing import Union


SYS_I2C = "/sys/bus/i2c/devices/"
LTC4151_PATH = SYS_I2C + "7-006f/hwmon/hwmon*/"
LTC4281_PATH = SYS_I2C + "7-004a/"
SYSCPLD_PATH = SYS_I2C + "12-0031/"
I2C_SET_BYTE = "/usr/sbin/i2cset -f -y %d 0x%x 0x%x"
I2C_SET_BYTE_DATA = "/usr/sbin/i2cset -f -y %d 0x%x 0x%x 0x%x"
I2C_GET_BYTE = "/usr/sbin/i2cget -f -y %d 0x%x"
I2C_DETECT_CMD = "i2cdetect -y %d"

INSTANTIATE_EEPROM_DRIVER_CMD = (
    "echo 24c02 0x%s > /sys/bus/i2c/devices/i2c-%d/new_device"
)
POWER_MODULE_TYPE = "echo %s: %d >> /tmp/detect_power_module_type.txt"
INSTANTIATE_PCA954X_CMD = "echo 7-0070 > /sys/bus/i2c/drivers/pca954x/bind 2> /dev/null"
PCA954X_BIND_PATH = "/sys/bus/i2c/drivers/pca954x"
PCA954X_BIND_PATH_ADDR = "/sys/bus/i2c/drivers/pca954x/7-0070"
READ_FRU_EEPROM_CMD = (
    "hexdump -C /sys/bus/i2c/devices/%d-00%d/eeprom | grep 00000040 | cut -d ' ' -f 17"
)
LTC4151_VIN_HWMON = LTC4151_PATH + "in1_input"
LTC4281_TEMP = LTC4281_PATH + "temp1_input"
LTC4281_FAULT_LOG_EN = LTC4281_PATH + "fault_log_enable"
LTC4281_ALERT_GENERATED = LTC4281_PATH + "alert_generated"
USERVER_POWER = SYSCPLD_PATH + "pwr_usrv_en"
MAIN_POWER = SYSCPLD_PATH + "pwr_main_n"
PSU_PRESENT = {0x1: SYSCPLD_PATH + "psu2_present", 0x2: SYSCPLD_PATH + "psu1_present"}
LTC4151_VIN_MINIMUM_THRESHOLD = 11050
LTC4151_VIN_CRITICAL_THRESHOLD = 10500
REMOVE_LTC4281_DRIVER = "modprobe -r ltc4281"
REMOVE_LTC4151_DRIVER = "modprobe -r ltc4151"

LTC4281_SENSOR = [
    ["Source_Voltage", "in0_input", "V"],
    ["Drain_Voltage", "in1_input", "V"],
    ["GPIO3_Voltage", "in2_input", "V"],
    ["Current", "curr1_input", "A"],
    ["Power", "power1_input", "W"],
]
LTC4281_STATUS1 = [
    "ov_status",
    "uv_status",
    "oc_cooldown_status",
    "power_good_status",
    "on_pin_status",
    "fet_short_present",
    "fet_bad_cooldown_status",
    "on_status",
]
LTC4281_STATUS2 = [
    "meter_overflow_present",
    "ticker_overflow_present",
    "adc_idle",
    "eeprom_busy",
    "alert_status",
    "gpio1_status",
    "gpio2_status",
    "gpio3_status",
]
LTC4281_FAULT_LOG = [
    "ov_fault",
    "uv_fault",
    "oc_fault",
    "power_bad_fault",
    "on_fault",
    "fet_short_fault",
    "fet_bad_fault",
    "eeprom_done",
]
LTC4281_ADC_ALERT_LOG = [
    "gpio_alarm_low",
    "gpio_alarm_high",
    "vsource_alarm_low",
    "vsource_alarm_high",
    "vsense_alarm_low",
    "vsense_alarm_high",
    "power_alarm_low",
    "power_alarm_high",
]
LTC4281_FAULT_LOG_EE = [
    "ov_fault_ee",
    "uv_fault_ee",
    "oc_fault_ee",
    "power_bad_fault_ee",
    "on_fault_ee",
    "fet_short_fault_ee",
    "fet_bad_fault_ee",
    "eeprom_done_ee",
]
LTC4281_ADC_ALERT_LOG_EE = [
    "gpio_alarm_low_ee",
    "gpio_alarm_high_ee",
    "vsource_alarm_low_ee",
    "vsource_alarm_high_ee",
    "vsense_alarm_low_ee",
    "vsense_alarm_high_ee",
    "power_alarm_low_ee",
    "power_alarm_high_ee",
]

MUX_BUS = 7
SYSCPLD_BUS = 12
MUX_ADDR = 0x70
LTC4281_ADDR = 0x4A
SYSCPLD_ADDR = 0x31
TEMP_THRESHOLD_UCR = 85
TEMP_THRESHOLD_UNR = 100
UCR = "Upper Critical"
UNR = "Upper Non Recoverable"
ADDR_LINE_TARGET = 6
PSU1_ADDR = "5a"
PSU2_ADDR = "59"
PEM1_ADDR = "55"
PEM2_ADDR = "56"
BUS_ADDR1 = 14
BUS_ADDR2 = 15
PSU1_ADDR_INDEX = 11
PSU2_ADDR_INDEX = 10
PEM1_ADDR_INDEX = 6
PEM2_ADDR_INDEX = 7
SECOND_CHANNEL = 0x2


def sysfs_read(inp) -> Union[int, None]:
    """
    Read value from sysfs path,
    if value is valid return integer value, else return none.
    """
    try:
        with open(inp, "r") as f:
            str_val = f.readline().rstrip("\n")
            if str_val.find("0x") is -1:
                val = int(str_val, 10)
            else:
                val = int(str_val, 16)
            return val
    except Exception:
        return None


def sysfs_write(inp, val) -> None:
    """
    Write value to sysfs path, if  write fail, record sysfs path to syslog.
    """
    try:
        with open(inp, "w") as f:
            f.write("%d\n" % val)
    except Exception:
        syslog.syslog(syslog.LOG_CRIT, "Cannot write to %s" % inp)


def set_mux_channel(ch) -> None:
    """
    Switch PSU mux channel.
    """
    cmd = I2C_SET_BYTE % (MUX_BUS, MUX_ADDR, ch)
    subprocess.call(cmd.split(" "))


def exec_check_output(cmd) -> int:
    """
    Execute system command and return output integer value.
    """
    p = subprocess.Popen(cmd.split(" "), stdout=subprocess.PIPE)
    outputs, _ = p.communicate(timeout=5)

    if p.returncode != 0:
        return -1

    out = outputs.decode()

    return int(out, 16)


def exec_check_return(cmd) -> bool:
    """
    Execute command and if execute success return true, else return false.
    """
    returncode = subprocess.Popen(cmd.split(" ")).wait(timeout=5)

    if returncode:
        syslog.syslog(syslog.LOG_CRIT, "execute {} fail".format(cmd))
        return False
    else:
        return True


class Pcard(object):
    def __init__(self):
        self.ltc4151_vin = {0x1: None, 0x2: None}
        self.ltc4151_pcard_channel = None
        self.ltc4151_avg_cnt = 10
        self.ltc4281_ucr_assert = {0x1: False, 0x2: False}
        self.ltc4281_unr_assert = {0x1: False, 0x2: False}
        self.ltc4281_fet_bad = {0x1: False, 0x2: False}
        self.ltc4281_fet_bad_cnt = {0x1: 0, 0x2: 0}
        self.ltc4281_temp_no_val_cnt = {0x1: 0, 0x2: 0}
        self.ltc4151_voltage_no_val_cnt = {0x1: 0, 0x2: 0}
        self.ltc4281_temp_no_val = {0x1: False, 0x2: False}
        self.ltc4281_temp_no_val_power_off = {0x1: False, 0x2: False}
        self.first_detect = {0x1: True, 0x2: True}
        self.with_ltc4281 = {0x1: False, 0x2: False}
        self.with_ltc4151 = {0x1: False, 0x2: False}
        self.present_status = {0x1: 1, 0x2: 1}
        self.ltc4281_driver_present = True
        self.present_channel = []
        self.first_run = False
        self.ltc4251_vin_critical_flag = False
        self.ltc4151_driver_present = True

    def power_consumption_reduction(self) -> None:
        """
        Power off userver and switching ASIC,
        reset QFFP, set fan PWM to 20% duty
        """
        syslog.syslog(syslog.LOG_CRIT, "Power consumption reduction start")
        # Power off userver
        sysfs_write(USERVER_POWER, 0)
        # Wait for a while to make sure command execute done
        time.sleep(1)

        # Power off switching ASIC
        sysfs_write(MAIN_POWER, 0)
        # Wait for a while to make sure command execute done
        time.sleep(1)

        # Set QSFP to reset mode
        for reg in [0x34, 0x35, 0x36, 0x37]:
            cmd = I2C_SET_BYTE_DATA % (SYSCPLD_BUS, SYSCPLD_ADDR, reg, 0x00)
            exec_check_return(cmd)
            # Wait for a while to make sure command execute done
            time.sleep(0.05)

        # Set QSFP to normal mode
        for reg in [0x34, 0x35, 0x36, 0x37]:
            cmd = I2C_SET_BYTE_DATA % (SYSCPLD_BUS, SYSCPLD_ADDR, reg, 0xFF)
            exec_check_return(cmd)
            # Wait for a while to make sure command execute done
            time.sleep(0.05)

        # Stop FSCD and watchdog
        exec_check_return("sv stop fscd")
        # Wait for a while to make sure command execute done
        time.sleep(1)
        exec_check_return("/usr/local/bin/wdtcli stop")
        # Wait for a while to make sure command execute done
        time.sleep(1)

        # Set FAN PWM duty to 20%
        cmd = "/usr/local/bin/set_fan_speed.sh 20"
        exec_check_return(cmd)
        syslog.syslog(syslog.LOG_CRIT, "Power consumption reduction finish")

    def present_detect(self) -> None:
        """
        Detect PSU slot1 present status(syscpld register 0x8 bit 1)
        and detect PSU slot2 present status(syscpld register 0x8 bit 0)
        """
        val = {}
        for ch in [0x1, 0x2]:
            # val = 1, means not present; val = 0,  means present
            val[ch] = sysfs_read(PSU_PRESENT[ch])
            if isinstance(val[ch], int):
                if val[ch] != self.present_status[ch]:
                    if val[ch] == 0:
                        self.present_channel.append(ch)
                        syslog.syslog(
                            syslog.LOG_CRIT, "Mux channel %d pcard/psu insertion" % ch
                        )
                    else:
                        self.present_channel.remove(ch)
                        self.first_detect[ch] = True
                        syslog.syslog(
                            syslog.LOG_CRIT, "Mux channel %d pcard/psu extraction" % ch
                        )
                        # Live extraction. Will never happen in DC, but Good to
                        # to have the logic because it does apply to 2 PEM case
                        self.ltc4151_vin[ch] = None
                self.present_status[ch] = val[ch]
            else:
                # nothing found. No channel will be appended to the list
                self.first_detect[ch] = True
                syslog.syslog(
                    syslog.LOG_CRIT, "Mux channel %d pcard/psu extraction" % ch
                )
                self.present_status[ch] = 1
                self.ltc4151_vin[ch] = None

    def userver_power_is_on(self) -> bool:
        """
        Detect both of userver power enable(syscpld register 0x30 bit 2)
        and main power enable(syscpld register 0x30 bit 1)
        """
        userver = sysfs_read(USERVER_POWER)
        main = sysfs_read(MAIN_POWER)

        if isinstance(userver, int) and isinstance(main, int):
            val = userver | main
            if val == 1:
                return True
            else:
                return False
        else:
            return False

    def get_ltc4151_hwmon_source(self) -> str:
        """
        Get LTC4151 sysfs absolute path.
        """
        hwmon_path = None
        result = re.split("hwmon", LTC4151_VIN_HWMON)
        if os.path.isdir(result[0]):
            construct_hwmon_path = result[0] + "hwmon"
            x = None
            for x in os.listdir(construct_hwmon_path):
                if x.startswith("hwmon"):
                    construct_hwmon_path = (
                        construct_hwmon_path + "/" + x + "/" + result[2].split("/")[1]
                    )
                    if os.path.exists(construct_hwmon_path):
                        hwmon_path = construct_hwmon_path
                        syslog.syslog(
                            syslog.LOG_INFO,
                            "Reading ltc pcard_ltc4151_vin={}".format(hwmon_path),
                        )
                        return hwmon_path

    def ltc4151_read(self, ch) -> Union[int, None]:
        """
        Read LTC4151 register value.
        """
        try:
            # Read the source path of the ltc4151
            # voltage from the sysfs path
            if not self.ltc4151_vin[ch]:
                self.ltc4151_vin[ch] = self.get_ltc4151_hwmon_source()
                syslog.syslog(
                    syslog.LOG_INFO,
                    "read path %s and channel %d" % (self.ltc4151_vin[ch], ch),
                )
            return sysfs_read(self.ltc4151_vin[ch])
        except Exception:
            return None

    def ltc4151_read_channel(self, ch) -> Union[int, None]:
        """
        Switch mux channel and read LTC4151 register value.
        """
        set_mux_channel(ch)
        return self.ltc4151_read(ch)

    def check_ltc4151_failed_read(self, ch) -> None:
        """
        We cannot read data behind the i2c mux when userver is off
        which would cause the voltage reading to fail, so we check
        up to 10 times if we are either dealing with a case where
        userver is off or if we are dealing with a case where userver
        is on. If userver is on and we still cannot read, the expected
        behavior is to reduce the load for safety precaution
        """
        self.ltc4151_voltage_no_val_cnt[ch] += 1
        if self.ltc4151_voltage_no_val_cnt[ch] >= 10:
            if self.userver_power_is_on():
                syslog.syslog(
                    syslog.LOG_CRIT,
                    "LTC4151: Channel %d voltage failed to read due to"
                    " ltc4151 gotten in unknown trouble. Reducing Power consumption..."
                    % ch,
                )
                self.power_consumption_reduction()
            else:
                syslog.syslog(
                    syslog.LOG_CRIT,
                    "LTC4151: Channel %d of voltage failed to"
                    " read due to userver being off" % ch,
                )
            # reset the counter and continue with the logic flow
            self.ltc4151_voltage_no_val_cnt[ch] = 0

    def exec_cmd(self, cmd_data) -> str:
        """
        execute cmd
        """
        output_data = os.popen(cmd_data)
        if output_data:
            syslog.syslog(syslog.LOG_CRIT, "running %s succeeded" % cmd_data)
        else:
            syslog.syslog(syslog.LOG_INFO, "running %s failed" % cmd_data)
        return output_data

    def detect_i2c_addr(self, eeprom_addr, bus_addr, addr_index) -> bool:
        # detect the eeprom addr of a specific fru
        syslog.syslog(
            syslog.LOG_INFO,
            "running detect_i2c_addr for device"
            " at bus %d addr 0x%s and i2c table index %d "
            % (bus_addr, eeprom_addr, addr_index),
        )
        busy_addr = "UU"

        i2cdata = self.exec_cmd(I2C_DETECT_CMD % bus_addr)

        i2cdata_list = i2cdata.readlines()
        addr_line = i2cdata_list[ADDR_LINE_TARGET].split()
        addr_read = addr_line[addr_index]
        syslog.syslog(syslog.LOG_INFO, "addr read is 0x%s" % addr_read)
        syslog.syslog(
            syslog.LOG_INFO,
            "addr_read = %s and eeprom_addr = %s" % (addr_read, eeprom_addr),
        )
        syslog.syslog(
            syslog.LOG_INFO,
            "addr_read = %s and busy_addr = %s" % (addr_read, busy_addr),
        )
        return addr_read == eeprom_addr or addr_read == busy_addr

    def generate_pcard_type_file(self, power_type) -> None:
        """
        Since we support only one DC PEM for wedge100. Changing
        The PEM is disruptive, so we need this function to run
        only once. /tmp/ directory will be clear upon reboot
        """
        if os.path.exists("/tmp/detect_power_module_type.txt"):
            self.exec_cmd("rm /tmp/detect_power_module_type.txt")
            self.exec_cmd("touch /tmp/detect_power_module_type.txt")
        self.exec_cmd(POWER_MODULE_TYPE % ("pem1", power_type["pem1"]))
        self.exec_cmd(POWER_MODULE_TYPE % ("pem2", power_type["pem2"]))
        self.exec_cmd(POWER_MODULE_TYPE % ("psu1", power_type["psu1"]))
        self.exec_cmd(POWER_MODULE_TYPE % ("psu2", power_type["psu2"]))

    def get_pem_fru_version(self, eeprom_addr, bus_addr) -> int:
        """
        execute the command that will return
        the fru version of the pem
        """
        syslog.syslog(
            syslog.LOG_INFO,
            "running get_pem_fru_version for device"
            " at bus %d and address 0x%s " % (bus_addr, eeprom_addr),
        )
        fru_cmd = READ_FRU_EEPROM_CMD % (bus_addr, int(eeprom_addr))
        syslog.syslog(syslog.LOG_INFO, "cmd to be run %s" % fru_cmd)
        try:
            # shell=True is causing linting. It doesn't wowrk without shell-True
            cmd_output = subprocess.Popen(
                fru_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
            data, err = cmd_output.communicate()
            fru_ver = data.decode("utf-8")
        except Exception:
            syslog.syslog(syslog.LOG_CRIT, "running cmd %d failed" % int(fru_cmd))
        syslog.syslog(syslog.LOG_INFO, "fru_version read is %d" % int(fru_ver))
        return int(fru_ver)

    def instantiate_eeprom_driver(self, eeprom_addr, bus_number) -> None:
        syslog.syslog(
            syslog.LOG_INFO,
            "instantiating eeprom driver for"
            " addr 0x%s and bus number %d" % (eeprom_addr, bus_number),
        )
        self.exec_cmd(INSTANTIATE_EEPROM_DRIVER_CMD % (eeprom_addr, bus_number))

    def detect_power_type(self, ch) -> int:
        """
        we can either one two AC PSU or 1 LTC4151 PEM in DC
        or an LTC4281 PEM in DC. So we need to use the channel
        to detect which power type that we are dealing with and
        then, the content of the FRU eeprom will be read to
        termine what's the PEM type
        """
        pem1_addr_present = False
        pem2_addr_present = False
        psu1_addr_present = False
        psu2_addr_present = False

        # Looping 10 times because PEM addr is not alwways detected
        for count in range(10):
            syslog.syslog(syslog.LOG_INFO, " detecting psu/pem type in try %d" % count)
            if len(self.present_channel) == 2:
                for channel_index in self.present_channel:
                    if channel_index == SECOND_CHANNEL:
                        psu1_addr_present = self.detect_i2c_addr(
                            PSU1_ADDR, BUS_ADDR1, PSU1_ADDR_INDEX
                        )
                    else:
                        psu2_addr_present = self.detect_i2c_addr(
                            PSU2_ADDR, BUS_ADDR2, PSU2_ADDR_INDEX
                        )
            else:
                # we use only 1 PEM per swicth
                if ch == SECOND_CHANNEL:
                    pem1_addr_present = self.detect_i2c_addr(
                        PEM1_ADDR, BUS_ADDR2, PEM1_ADDR_INDEX
                    )
                else:
                    pem2_addr_present = self.detect_i2c_addr(
                        PEM2_ADDR, BUS_ADDR1, PEM2_ADDR_INDEX
                    )

            # No need to continue the iteration if a power type is detected
            if (
                pem1_addr_present
                or pem2_addr_present
                or psu1_addr_present
                or psu2_addr_present
            ):
                break

        if psu1_addr_present and psu2_addr_present:
            syslog.syslog(
                syslog.LOG_INFO,
                "PSU1 found at bus %d and addr 0x%s"
                " and psu2 found at bus %d and addr 0x%s"
                % (BUS_ADDR1, PSU1_ADDR, BUS_ADDR2, PSU2_ADDR),
            )
            fru_ver = -1
        elif psu1_addr_present or psu2_addr_present:
            if psu1_addr_present:
                syslog.syslog(
                    syslog.LOG_INFO,
                    "PSU1 found at bus %d and addr 0x%s" % (BUS_ADDR1, PSU1_ADDR),
                )
                fru_ver = -2
            else:
                syslog.syslog(
                    syslog.LOG_INFO,
                    "PSU2 found at bus %d and addr 0x%s" % (BUS_ADDR2, PSU2_ADDR),
                )
                fru_ver = -3
        elif pem1_addr_present:
            syslog.syslog(
                syslog.LOG_INFO,
                "PEM1 detected at bus %d and addr 0x%s" % (BUS_ADDR2, PEM1_ADDR),
            )
            self.instantiate_eeprom_driver(PEM1_ADDR, BUS_ADDR2)
            fru_ver = self.get_pem_fru_version(PEM1_ADDR, BUS_ADDR2)
        elif pem2_addr_present:
            syslog.syslog(
                syslog.LOG_INFO,
                "PEM2 detected at bus %d and addr 0x%s" % (BUS_ADDR1, PEM2_ADDR),
            )
            self.instantiate_eeprom_driver(PEM2_ADDR, BUS_ADDR1)
            fru_ver = self.get_pem_fru_version(PEM2_ADDR, BUS_ADDR1)
        else:
            syslog.syslog(syslog.LOG_CRIT, "No Power Device Detected")
            fru_ver = -4
        return fru_ver

    def ltc4281_detect(self, ch) -> None:
        """
        Switch mux channel and read PEM fru eeprom Product Version,
        fru value = 1 means PEM without LTC4281,
        fru value >= 2 means PEM with LTC4281.
        fru value = -1 means both psu1 and psu2 are present
        fru value = -2 means psu1 present only
        fru value = -3 means psu2 present only
        fru value = -4 no power detected even after looping 10 times
        pem1 = OLD PEM with ltc4151
        pem2 = New PEM with ltc4281
        There will always be only 1 PEM in DC (Never 2)
        """
        syslog.syslog(syslog.LOG_INFO, "running ltc4281_detect to find hotswap type")
        power_type = {"pem1": 0, "pem2": 0, "psu1": 0, "psu2": 0}
        fru_ver = self.detect_power_type(ch)
        if fru_ver == -1:
            syslog.syslog(syslog.LOG_CRIT, "Mux channel %d is ac psu" % fru_ver)
            self.with_ltc4281[ch] = False
            self.with_ltc4151[ch] = False
            power_type["psu1"] = 1
            power_type["psu2"] = 1
        elif fru_ver == 1:
            syslog.syslog(
                syslog.LOG_CRIT,
                "Mux channel %d is pcard with LTC4151(type=%d)" % (ch, fru_ver),
            )
            self.with_ltc4281[ch] = False
            self.with_ltc4151[ch] = True
            power_type["pem1"] = 1
        elif fru_ver >= 2:
            syslog.syslog(
                syslog.LOG_CRIT,
                "Mux channel %d is pcard with LTC4281(type=%d)" % (ch, fru_ver),
            )
            self.with_ltc4151[ch] = False
            power_type["pem2"] = 1
            if not self.check_ltc4281_status(ch):
                syslog.syslog(
                    syslog.LOG_CRIT, "Mux channel %d fault status is detected" % ch
                )
                self.dump_ltc4281_status(ch)
                self.clear_ltc4281_status(ch)
            self.init_ltc4281(ch)
            self.with_ltc4281[ch] = True
        elif fru_ver == -2:
            syslog.syslog(syslog.LOG_INFO, "Only AC psu1 is detected")
            power_type["psu1"] = 1
        elif fru_ver == -3:
            syslog.syslog(syslog.LOG_INFO, "Only AC psu2 is detected")
        else:
            syslog.syslog(
                syslog.LOG_CRIT,
                "No power device could be detected"
                " reducing power load for safety reasons",
            )
            self.power_consumption_reduction()

        # Generate/Add entry to file that contains
        # power module type
        self.generate_pcard_type_file(power_type)

    def ltc4281_read_channel(self, ch, inp) -> Union[int, None]:
        """
        Switch mux channel and read LTC4281 register value.
        """
        set_mux_channel(ch)
        return sysfs_read(inp)

    def ltc4281_write_channel(self, ch, inp, val) -> None:
        """
        Switch mux channel and write LTC4281 register value.
        """
        set_mux_channel(ch)
        sysfs_write(inp, val)

    def ltc4281_temp_fail_assert(self, ch) -> None:
        """
        Save assert log caused by temperature reading fail
        """
        self.ltc4281_temp_no_val_cnt[ch] += 1
        if self.ltc4281_temp_no_val_cnt[ch] >= 10:
            if self.userver_power_is_on():
                if not self.ltc4281_temp_no_val[ch]:
                    syslog.syslog(
                        syslog.LOG_CRIT,
                        "ASSERT: raised - channel %d temp failed to read "
                        "due to LTC4281 get unknown trouble" % ch,
                    )
                    self.power_consumption_reduction()
                    self.ltc4281_temp_no_val[ch] = True
            else:
                if (
                    not self.ltc4281_temp_no_val_power_off[ch]
                    and not self.ltc4281_temp_no_val[ch]
                ):
                    syslog.syslog(
                        syslog.LOG_CRIT,
                        "ASSERT: raised - channel %d temp failed to read "
                        "due to wedge power is off" % ch,
                    )
                    self.ltc4281_temp_no_val_power_off[ch] = True

    def ltc4281_temp_fail_deassert(self, ch) -> None:
        """
        Save deassert log for temperature is back to read
        """
        if self.ltc4281_temp_no_val_power_off[ch]:
            syslog.syslog(
                syslog.LOG_CRIT,
                "DEASSERT: settled - channel %d LTC4281 temp success "
                "to read, wedge power is back on" % ch,
            )
            self.ltc4281_temp_no_val_power_off[ch] = False

        if self.ltc4281_temp_no_val[ch]:
            syslog.syslog(
                syslog.LOG_CRIT,
                "DEASSERT: settled - channel %d LTC4281 temp success "
                "to read, unknown trouble is fixed" % ch,
            )
            self.ltc4281_temp_no_val[ch] = False
        self.ltc4281_temp_no_val_cnt[ch] = 0

    def ltc4281_temp_ucr_unr_check(self, ch, temp) -> None:
        """
        Temperature >= 85, record assert log,
        temperature is from >= 85 to < 85, record deassert log.
        Temperature >= 100, reduce power comsumption and record assert log,
        temperature is from >= 100 to < 100, record deassert log.
        """
        if temp >= TEMP_THRESHOLD_UNR:
            if not self.ltc4281_unr_assert[ch]:
                syslog.syslog(
                    syslog.LOG_CRIT,
                    "ASSERT: %s threshold - "
                    "raised - channel: %d, MOSFET Temp curr_val: %.2f C, "
                    "thresh_val: %.2f C" % (UNR, ch, temp, TEMP_THRESHOLD_UNR),
                )
                self.dump_ltc4281_status(ch)
                self.power_consumption_reduction()
                self.ltc4281_unr_assert[ch] = True
        elif temp < TEMP_THRESHOLD_UNR and temp >= TEMP_THRESHOLD_UCR:
            if self.ltc4281_unr_assert[ch]:
                syslog.syslog(
                    syslog.LOG_CRIT,
                    "DEASSERT: %s threshold - "
                    "settled - channel: %d, MOSFET Temp curr_val: %.2f C, "
                    "thresh_val: %.2f C" % (UNR, ch, temp, TEMP_THRESHOLD_UNR),
                )
                self.dump_ltc4281_status(ch)
                self.ltc4281_unr_assert[ch] = False

            if not self.ltc4281_ucr_assert[ch]:
                syslog.syslog(
                    syslog.LOG_CRIT,
                    "ASSERT: %s threshold - "
                    "raised - channel: %d, MOSFET Temp curr_val: %.2f C, "
                    "thresh_val: %.2f C" % (UCR, ch, temp, TEMP_THRESHOLD_UCR),
                )
                self.dump_ltc4281_status(ch)
                self.ltc4281_ucr_assert[ch] = True
        else:
            if self.ltc4281_unr_assert[ch]:
                syslog.syslog(
                    syslog.LOG_CRIT,
                    "DEASSERT: %s threshold - "
                    "settled - channel: %d, MOSFET Temp curr_val: %.2f C, "
                    "thresh_val: %.2f C" % (UNR, ch, temp, TEMP_THRESHOLD_UNR),
                )
                self.dump_ltc4281_status(ch)
                self.ltc4281_unr_assert[ch] = False

            if self.ltc4281_ucr_assert[ch]:
                syslog.syslog(
                    syslog.LOG_CRIT,
                    "DEASSERT: %s threshold - "
                    "settled - channel: %d, MOSFET Temp curr_val: %.2f C, "
                    "thresh_val: %.2f C" % (UCR, ch, temp, TEMP_THRESHOLD_UCR),
                )
                self.dump_ltc4281_status(ch)
                self.ltc4281_ucr_assert[ch] = False

    def check_ltc4281_temp(self, ch) -> None:
        """
        Check temperature value is valid or not,
        if value is valid, check it is over threshold or not.
        """
        temp = self.ltc4281_read_channel(ch, LTC4281_TEMP)

        if temp is None:
            self.ltc4281_temp_fail_assert(ch)
        else:
            self.ltc4281_temp_fail_deassert(ch)
            temp = temp / 1000
            self.ltc4281_temp_ucr_unr_check(ch, temp)

    def check_ltc4281_fet(self, ch) -> None:
        """
        If one of MOSFET status is bad,
        reduce power comsumption and record assert log.
        All MOSFET status is normal now and have occoured MOSFET bad before,
        record deassert log.
        """
        fet_short = self.ltc4281_read_channel(
            ch, os.path.join(LTC4281_PATH, "fet_short_present")
        )
        fet_bad_cool = self.ltc4281_read_channel(
            ch, os.path.join(LTC4281_PATH, "fet_bad_cooldown_status")
        )
        fet_bad_fault = self.ltc4281_read_channel(
            ch, os.path.join(LTC4281_PATH, "fet_bad_fault")
        )
        fet_bad_fault_ee = self.ltc4281_read_channel(
            ch, os.path.join(LTC4281_PATH, "fet_bad_fault_ee")
        )

        if (
            isinstance(fet_short, int)
            and isinstance(fet_bad_cool, int)
            and isinstance(fet_bad_fault, int)
            and isinstance(fet_bad_fault_ee, int)
        ):
            val = fet_short | fet_bad_cool | fet_bad_fault | fet_bad_fault_ee
            if val == 1:
                self.ltc4281_fet_bad_cnt[ch] += 1
                if self.ltc4281_fet_bad_cnt[ch] >= 5 and not self.ltc4281_fet_bad[ch]:
                    syslog.syslog(
                        syslog.LOG_CRIT,
                        "ASSERT: raised - channel: " "%d, MOSFET status is bad" % ch,
                    )
                    self.dump_ltc4281_status(ch)
                    self.power_consumption_reduction()
                    self.ltc4281_fet_bad[ch] = True
            else:
                if self.ltc4281_fet_bad[ch]:
                    syslog.syslog(
                        syslog.LOG_CRIT,
                        "DEASSERT: settled - "
                        "channel: %d, MOSFET status is good" % ch,
                    )
                    self.dump_ltc4281_status(ch)
                    self.ltc4281_fet_bad[ch] = False
                    self.ltc4281_fet_bad_cnt[ch] = 0

    def check_ltc4151_vin_threshold(self, ch, vin) -> None:
        """
        check if the voltage is below the threshold. if
        if is <= 10V, we will reduce the load. However,
        if it is between 10 and 11v, we will compute the
        moving avg of 10 sample. Then we will check to see
        if it is below the 11v so that we can reduce the load
        """
        if vin < LTC4151_VIN_MINIMUM_THRESHOLD:
            syslog.syslog(
                syslog.LOG_CRIT,
                "LTC4151: voltage of %d  below minimum threshhold of 11.05V detected"
                % vin,
            )
            voltage_avg = 0
            while self.ltc4151_avg_cnt > 0:
                syslog.syslog(
                    syslog.LOG_INFO,
                    "LTC4151: computing avg with  volt = %d and counter = %d "
                    % (vin, self.ltc4151_avg_cnt),
                )
                # if we get a reading below 11V and the previous one was <= 10.5,
                # we will reduce the load immediately and then return.
                if (
                    vin <= LTC4151_VIN_MINIMUM_THRESHOLD
                    and self.ltc4251_vin_critical_flag
                ):
                    syslog.syslog(
                        syslog.LOG_CRIT,
                        "LTC4151 Reducing load: voltage = %d after previous "
                        "voltage below minimum threshold which is set at 10.5V "
                        "and counter = % d" % (vin, self.ltc4151_avg_cnt),
                    )
                    self.ltc4251_vin_critical_flag = False
                    self.power_consumption_reduction()
                    self.ltc4151_avg_cnt = 10
                    return
                elif self.ltc4251_vin_critical_flag:
                    self.ltc4251_vin_critical_flag = False

                # checking for voltage below 10.50V
                if vin < LTC4151_VIN_CRITICAL_THRESHOLD:
                    syslog.syslog(
                        syslog.LOG_CRIT,
                        "LTC4151: Voltage of %d is below the critical threshold of "
                        "10.50V and counter = %d" % (vin, self.ltc4151_avg_cnt),
                    )
                    self.ltc4251_vin_critical_flag = True
                voltage_avg += vin
                time.sleep(5)  # sleep for 5 seconds between avg iteration
                vin = self.ltc4151_read_channel(ch)
                # in case voltage reading fail, we will iterate up to 10 times
                while not vin:
                    self.check_ltc4151_failed_read(ch)
                    time.sleep(5)
                    vin = self.ltc4151_read_channel(ch)

                self.ltc4151_avg_cnt -= 1
                self.ltc4151_voltage_no_val_cnt[
                    ch
                ] = 0  # reading succeeded counter must be 0
            # Reset the avg count and reduce the load if necessary
            self.ltc4151_avg_cnt = 10
            voltage_avg /= self.ltc4151_avg_cnt
            if voltage_avg < LTC4151_VIN_MINIMUM_THRESHOLD:
                syslog.syslog(
                    syslog.LOG_CRIT,
                    "LTC4151 Reducing load: Avg Volt = %d and counter = %d"
                    % (voltage_avg, self.ltc4151_avg_cnt),
                )
                self.power_consumption_reduction()

    def check_ltc4151_vol(self) -> None:
        """
        Check LTC4151 voltage value
        """
        live_channels = []
        for ch in self.present_channel:
            vin = self.ltc4151_read_channel(ch)
            if vin:
                live_channels.append(ch)
                self.ltc4151_voltage_no_val_cnt[
                    ch
                ] = 0  # reading succeeded counter must be 0
                self.check_ltc4151_vin_threshold(ch, vin)
            else:
                self.check_ltc4151_failed_read(
                    ch
                )  # will check userver status up to 10 times
        if len(live_channels) == 0:
            syslog.syslog(
                syslog.LOG_WARNING,
                "No ltc4151 (passthrough " "card voltage monitor) detected",
            )
            self.ltc4151_pcard_channel = None
        if len(live_channels) == 1:
            ch = live_channels[0]
            if self.ltc4151_pcard_channel is None:
                syslog.syslog(
                    syslog.LOG_INFO,
                    "Passsthrough card " "ltc4151 detected on mux channel %d" % ch,
                )
                self.ltc4151_pcard_channel = ch
                set_mux_channel(self.ltc4151_pcard_channel)
        if len(live_channels) == 2:
            for ch in live_channels:
                if self.ltc4151_pcard_channel != ch:
                    self.ltc4151_pcard_channel = ch
            syslog.syslog(
                syslog.LOG_INFO,
                "ltc4151 detected on both mux channels, on channel %d "
                "for next %d seconds..."
                % (self.ltc4151_pcard_channel, self.ltc4151_check_interval),
            )
            set_mux_channel(self.ltc4151_pcard_channel)

    def check_ltc4281_status(self, ch, delay=0.01) -> bool:
        """
        Check LTC4281 status register (0x04, 0x05, 0x24, 0x25, 0x1e)
        """
        log_status = (
            LTC4281_FAULT_LOG
            + LTC4281_FAULT_LOG_EE
            + LTC4281_ADC_ALERT_LOG
            + LTC4281_ADC_ALERT_LOG_EE
        )

        for status in log_status:
            val = self.ltc4281_read_channel(ch, os.path.join(LTC4281_PATH, status))
            if isinstance(val, int) and val == 1:
                return False
            # Wait for a while to release cpu usage
            time.sleep(delay)

        for index in range(len(LTC4281_STATUS1)):
            val = self.ltc4281_read_channel(
                ch, os.path.join(LTC4281_PATH, LTC4281_STATUS1[index])
            )
            if index == 3 or index == 4 or index == 7:
                if isinstance(val, int) and val is 0:
                    return False
            else:
                if isinstance(val, int) and val == 1:
                    return False
            # Wait for a while to release cpu usage
            time.sleep(delay)

        for index in range(len(LTC4281_STATUS2)):
            val = self.ltc4281_read_channel(
                ch, os.path.join(LTC4281_PATH, LTC4281_STATUS2[index])
            )
            if index == 4 or index == 5 or index == 6:
                if isinstance(val, int) and val is 0:
                    return False
            else:
                if isinstance(val, int) and val == 1:
                    return False
            # Wait for a while to release cpu usage
            time.sleep(delay)

        return True

    def dump_ltc4281_status(self, ch, delay=0.01) -> None:
        """
        Dump source voltage, drain voltage, GPIO3 voltage, current and power.
        Dump all LTC4281 status register
        (0x1e, 0x1f, 0x04, 0x05, 0x24, 0x25) value.
        """
        record = []
        for status in LTC4281_SENSOR:
            val = self.ltc4281_read_channel(ch, os.path.join(LTC4281_PATH, status[1]))
            if isinstance(val, int):
                val = "%.2f" % (val / 1000) + " " + status[2]
            record.append((status[0] + ": " + val))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog.syslog(
            syslog.LOG_CRIT, "Mux channel %d dump LTC4281 sensors %s" % (ch, record)
        )

        record = []
        for status in LTC4281_STATUS1:
            val = self.ltc4281_read_channel(ch, os.path.join(LTC4281_PATH, status))
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog.syslog(
            syslog.LOG_CRIT,
            "Mux channel %d dump LTC4281 status1 - " "%s at reg: 0x1e" % (ch, record),
        )

        record = []
        for status in LTC4281_STATUS2:
            val = self.ltc4281_read_channel(ch, os.path.join(LTC4281_PATH, status))
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog.syslog(
            syslog.LOG_CRIT,
            "Mux channel %d dump LTC4281 status2 - " "%s at reg: 0x1f" % (ch, record),
        )

        record = []
        for status in LTC4281_FAULT_LOG:
            val = self.ltc4281_read_channel(ch, os.path.join(LTC4281_PATH, status))
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog.syslog(
            syslog.LOG_CRIT,
            "Mux channel %d dump LTC4281 fault log " "- %s at reg: 0x04" % (ch, record),
        )

        record = []
        for status in LTC4281_ADC_ALERT_LOG:
            val = self.ltc4281_read_channel(ch, os.path.join(LTC4281_PATH, status))
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog.syslog(
            syslog.LOG_CRIT,
            "Mux channel %d dump LTC4281 adc alert "
            "log - %s at reg: 0x05" % (ch, record),
        )

        record = []
        for status in LTC4281_FAULT_LOG_EE:
            val = self.ltc4281_read_channel(ch, os.path.join(LTC4281_PATH, status))
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog.syslog(
            syslog.LOG_CRIT,
            "Mux channel %d dump LTC4281 fault log "
            "ee - %s at reg: 0x24" % (ch, record),
        )

        record = []
        for status in LTC4281_ADC_ALERT_LOG_EE:
            val = self.ltc4281_read_channel(ch, os.path.join(LTC4281_PATH, status))
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog.syslog(
            syslog.LOG_CRIT,
            "Mux channel %d dump LTC4281 adc alert "
            "log ee - %s at reg: 0x25" % (ch, record),
        )

    def clear_ltc4281_status(self, ch, delay=0.05) -> None:
        """
        Clear LTC4281 status register (0x04, 0x05, 0x24, 0x25) value.
        """
        syslog.syslog(syslog.LOG_CRIT, "Mux channel %d LTC4281 status clean start" % ch)
        log_status = (
            LTC4281_FAULT_LOG_EE
            + LTC4281_ADC_ALERT_LOG_EE
            + LTC4281_FAULT_LOG
            + LTC4281_ADC_ALERT_LOG
        )
        for status in log_status:
            self.ltc4281_write_channel(ch, os.path.join(LTC4281_PATH, status), 0)
            # Wait for a while to make sure command execute done
            time.sleep(delay)
        syslog.syslog(
            syslog.LOG_CRIT, "Mux channel %d LTC4281 status clean finish" % ch
        )

    def init_ltc4281(self, ch, delay=0.05) -> None:
        """
        Initialize LTC4281: enable fault log
        """
        syslog.syslog(
            syslog.LOG_CRIT, "Mux channel %d LTC4281 initialization start" % ch
        )
        self.ltc4281_write_channel(ch, LTC4281_FAULT_LOG_EN, 1)
        # Wait for a while to make sure command execute done
        time.sleep(delay)
        # Clear this bit to default, because it will be set to 1 when we do
        # clear_ltc4281_status function (triggered by EEPROM_DONE_ALERT).
        self.ltc4281_write_channel(ch, LTC4281_ALERT_GENERATED, 0)
        syslog.syslog(
            syslog.LOG_CRIT, "Mux channel %d LTC4281 initialization finish" % ch
        )

    def run(self) -> None:
        """
        Main psumuxmon method
        """
        # bind pca driver to 7-0070
        if os.path.exists(PCA954X_BIND_PATH) and not os.path.exists(
            PCA954X_BIND_PATH_ADDR
        ):
            syslog.syslog(syslog.LOG_INFO, "Binding pca954x driver")
            self.exec_cmd(INSTANTIATE_PCA954X_CMD)
        else:
            syslog.syslog(syslog.LOG_INFO, "pca954x driver already binded")
        time.sleep(5)
        while True:
            self.present_detect()
            for ch in self.present_channel:
                if self.first_detect[ch]:
                    if self.userver_power_is_on():
                        self.ltc4281_detect(ch)
                        self.first_detect[ch] = False

                if self.with_ltc4281[ch]:  # New PEM
                    self.check_ltc4281_temp(ch)
                    self.check_ltc4281_fet(ch)
                elif self.with_ltc4151[ch]:  # Old PEM
                    # LTC4281 driver module always loaded at boot up,
                    # removing it will remove N/A field in sensor field
                    if self.ltc4281_driver_present:
                        syslog.syslog(
                            syslog.LOG_INFO,
                            "OLD PEM rev detected, so removing ltc4281 driver",
                        )
                        self.exec_cmd(REMOVE_LTC4281_DRIVER)
                        self.ltc4281_driver_present = False
                    self.check_ltc4151_vol()
                else:  # switch with AC PSU detected, so remove both
                    # ltc4281 and LTC4551 drivers to remove N/A
                    if self.ltc4281_driver_present:
                        syslog.syslog(
                            syslog.LOG_INFO, "AC PSU detected, remove ltc4281 driver"
                        )
                        self.exec_cmd(REMOVE_LTC4281_DRIVER)
                        self.ltc4281_driver_present = False
                    if self.ltc4151_driver_present:
                        syslog.syslog(
                            syslog.LOG_INFO, "AC PSU detected, remove ltc4151 driver"
                        )
                        self.exec_cmd(REMOVE_LTC4151_DRIVER)
                        self.ltc4151_driver_present = False
            # Every 5s polling once
            time.sleep(5)


if __name__ == "__main__":
    syslog.openlog("psumuxmon")
    pcard = Pcard()
    pcard.run()
