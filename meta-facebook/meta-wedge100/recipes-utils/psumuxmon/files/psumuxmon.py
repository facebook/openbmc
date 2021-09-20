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

#
# psumuxmon is a daemon process for monitoring PEM devices.
#
# A Wedge100 could be powered by 1 PEM or 1/2 AC PSUs. PEM and PSU will
# never co-exist on a single switch, and we don't connect 2 PEMs to a
# single switch, either.
#
#   - PEM:
#     There are 2 PEM revisions. The first revision with LTC4151 chip,
#     and the newer (2nd) revision with both LTC4151 and LTC4281.
#     LTC4151 is included in the new PEMs for backward compatibility,
#     and software should ignore LTC4151 when LTC4281 is available.
#
#   - PSU:
#     If Wedge100 is powered by AC PSUs, then usually both 2 PSUs are
#     plugged. But we do have a few switches in basset pool with only
#     1 PSU connected.
#     psumuxmon doesn't support PSU monitoring at present, thus the
#     daemon will quit in this case.
#

import os
import re
import subprocess
import sys
import time
from syslog import openlog, syslog, LOG_CRIT, LOG_ERR, LOG_INFO
from typing import Union


#
# Cache power type in below file for easy access.
# Note: the path should be consistent with the one used in rest_presence.py
#
POWER_TYPE_CACHE = "/var/cache/detect_power_module_type.txt"

SYS_I2C_DEVICES = "/sys/bus/i2c/devices/"
SYS_I2C_DRIVERS = "/sys/bus/i2c/drivers/"
I2C_MUX_DEVICE = "7-0070"
I2C_MUX_DRIVER = "pca954x"
SYSCPLD_BUS = 12
SYSCPLD_ADDR = 0x31
SYSCPLD_PATH = os.path.join(SYS_I2C_DEVICES, "12-0031/")

I2C_SET_BYTE_DATA = "/usr/sbin/i2cset -f -y %d 0x%x 0x%x 0x%x"

LTC4151_HWMON_VIN = "in1_input"
LTC4151_VIN_MIN_THRESHOLD = 11050
LTC4151_VIN_CRIT_THRESHOLD = 10500

LTC4281_TEMP = "temp1_input"
LTC4281_FAULT_LOG_EN = "fault_log_enable"
LTC4281_ALERT_GENERATED = "alert_generated"

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

TEMP_THRESHOLD_UCR = 85
TEMP_THRESHOLD_UNR = 100
UCR_ALARM_NAME = "Upper Critical"
UNR_ALARM_NAME = "Upper Non Recoverable"


def sysfs_i2c_name(bus, addr) -> str:
    """
    Generate i2c device name in sysfs namespace.
    """
    return "%d-00%02x" % (bus, addr)


def sysfs_i2c_path(bus, addr) -> str:
    """
    Generate i2c device path in sysfs namespace.
    """
    name = sysfs_i2c_name(bus, addr)
    return os.path.join(SYS_I2C_DEVICES, name)


def sysfs_read_int(inp) -> Union[int, None]:
    """
    Read value from sysfs path,
    if value is valid return integer value, else return none.
    """
    try:
        with open(inp, "r") as f:
            str_val = f.readline().rstrip("\n")
            if str_val.find("0x") == -1:
                val = int(str_val, 10)
            else:
                val = int(str_val, 16)
            return val
    except Exception:
        return None


def sysfs_write_int(inp, val) -> None:
    """
    Write value to sysfs path, if  write fail, record sysfs path to syslog.
    """
    try:
        with open(inp, "w") as f:
            f.write("%d\n" % val)
    except Exception:
        syslog(LOG_ERR, "Cannot write to %s" % inp)


def syscpld_read_int(entry) -> Union[int, None]:
    """
    Read integer from the given SYSCPLD entry.
    """
    pathname = os.path.join(SYSCPLD_PATH, entry)
    return sysfs_read_int(pathname)


def syscpld_write_int(entry, val) -> None:
    """
    Write the given value to the according syscpld entry.
    """
    pathname = os.path.join(SYSCPLD_PATH, entry)
    sysfs_write_int(pathname, val)


def run_shell_cmd(cmd_data) -> str:
    """
    execute a shell command.
    """
    output_data = os.popen(cmd_data)
    if not output_data:
        syslog(LOG_INFO, "running %s failed" % cmd_data)
    return output_data


def exec_check_return(cmd) -> bool:
    """
    Execute command and if execute success return true, else return false.
    """
    returncode = subprocess.Popen(cmd.split(" ")).wait(timeout=5)

    if returncode:
        syslog(LOG_ERR, "execute {} fail".format(cmd))
        return False
    else:
        return True


def i2c_driver_bind(driver, device) -> None:
    """
    Bind the driver to a specific I2C device.
    """
    syslog(LOG_INFO, "manually bind %s driver to %s" % (driver, device))

    bind_path = os.path.join(SYS_I2C_DRIVERS, driver, "bind")
    cmd = "echo %s > %s" % (device, bind_path)
    run_shell_cmd(cmd)


def i2c_device_is_created(bus, addr) -> bool:
    """
    Test if an i2c device was already created.
    """
    pathname = sysfs_i2c_path(bus, addr)
    return os.path.exists(pathname)


def i2c_device_new(bus, addr, name) -> None:
    """
    Create a new I2C device.
    """
    if not i2c_device_is_created(bus, addr):
        syslog(LOG_INFO, "create %s (%s)" % (name, sysfs_i2c_name(bus, addr)))

        bus_entry = "i2c-%d" % bus
        new_dev_path = os.path.join(SYS_I2C_DEVICES, bus_entry, "new_device")
        cmd = "echo %s %s > %s" % (name, addr, new_dev_path)
        run_shell_cmd(cmd)


def i2c_device_detect(bus, addr) -> bool:
    """
    Check if a device is present by running i2cdetect.
    """
    cmd = "i2cdetect -y %d %d %d" % (bus, addr, addr)
    i2cdata = run_shell_cmd(cmd)
    i2cdata_list = i2cdata.readlines()

    row = int(addr / 16) + 1
    addr_line = i2cdata_list[row].split()
    dev_state = addr_line[1]

    #
    # Note: (dev_state == "UU") doesn't always mean the device is present:
    # it just means a driver is bound to the device. As we don't have an
    # easy way to do futher check, let's just assume the device is present
    # in this case.
    #
    return dev_state != "--"


class Pcard(object):
    def __init__(self):
        self.userver_is_powered = True
        self.ltc4151_info = {
            "present": False,
            "sysfs_path": None,
            "hwmon_path": None,
            "average_cnt": 10,
        }
        self.ltc4281_info = {
            "present": False,
            "sysfs_path": None,
            "hwmon_path": None,
            "ucr_assert": False,
            "unr_assert": False,
            "temp_read_error_cnt": 0,
            "temp_read_error_alarm": False,
            "temp_upper_crit_alarm": False,
            "temp_unrecoverable_alarm": False,
            "fet_bad_cnt": 0,
            "fet_bad_alarm": False,
        }
        self.power_supplies = {
            "psu1": {
                "present": False,
                "i2c_bus": 15,
                "eeprom_type": "24c64",
                "eeprom_addr": 0x51,
                "pwr_type": "pfe1100",
                "pwr_addr": 0x59,
            },
            "psu2": {
                "present": False,
                "i2c_bus": 14,
                "eeprom_type": "24c64",
                "eeprom_addr": 0x52,
                "pwr_type": "pfe1100",
                "pwr_addr": 0x5A,
            },
            "pem1": {
                "present": False,
                "i2c_bus": 15,
                "eeprom_type": "24c64",
                "eeprom_addr": 0x55,
                "pwr_type": None,
                "pwr_addr": None,
                "ltc4151": 0x6F,
                "ltc4281": 0x4A,
            },
            "pem2": {
                "present": False,
                "i2c_bus": 14,
                "eeprom_type": "24c64",
                "eeprom_addr": 0x56,
                "pwr_type": None,
                "pwr_addr": None,
                "ltc4151": 0x6F,
                "ltc4281": 0x4A,
            },
        }

    def psu_probe(self, psu_id) -> None:
        """
        Probe and initialize a PSU device.
        """
        p_info = self.power_supplies[psu_id]

        bus = p_info["i2c_bus"]
        eeprom_addr = p_info["eeprom_addr"]
        eeprom_type = p_info["eeprom_type"]

        if i2c_device_detect(bus, eeprom_addr):
            syslog(LOG_INFO, "%s is detected" % psu_id)
            p_info["present"] = True
            i2c_device_new(bus, eeprom_addr, eeprom_type)

            pwr_addr = p_info["pwr_addr"]
            pwr_type = p_info["pwr_type"]
            i2c_device_new(bus, pwr_addr, pwr_type)

    def pem_probe(self, pem_id) -> None:
        """
        Probe and initialize a PEM device.
        """
        p_info = self.power_supplies[pem_id]

        bus = p_info["i2c_bus"]
        eeprom_addr = p_info["eeprom_addr"]
        eeprom_type = p_info["eeprom_type"]

        if i2c_device_detect(bus, eeprom_addr):
            syslog(LOG_INFO, "%s is detected" % pem_id)
            p_info["present"] = True
            i2c_device_new(bus, eeprom_addr, eeprom_type)

            # Probe LTC4281 at first.
            if i2c_device_detect(bus, p_info["ltc4281"]):
                p_info["pwr_type"] = "ltc4281"
                p_info["pwr_addr"] = p_info["ltc4281"]
                i2c_device_new(bus, p_info["pwr_addr"], p_info["pwr_type"])

                self.ltc4281_info["present"] = True
                self.ltc4281_info["sysfs_path"] = sysfs_i2c_path(
                    bus, p_info["pwr_addr"]
                )

            # We will create ltc4151 device as long as it's reachable, but
            # we don't monitor it if ltc4281 is present.
            if i2c_device_detect(bus, p_info["ltc4151"]):
                p_info["pwr_type"] = "ltc4151"
                p_info["pwr_addr"] = p_info["ltc4151"]
                i2c_device_new(bus, p_info["pwr_addr"], p_info["pwr_type"])

                self.ltc4151_info["sysfs_path"] = sysfs_i2c_path(
                    bus, p_info["pwr_addr"]
                )

                # Create LTC4151 but don't monitor it if LTC4281 is available.
                if self.ltc4281_info["present"]:
                    syslog(LOG_INFO, "ignore ltc4151 as ltc4281 is available")
                else:
                    self.ltc4151_info["present"] = True

            if not self.ltc4151_info["present"] and not self.ltc4281_info["present"]:
                syslog(LOG_ERR, "%s: ltc4xxx is not reachable!" % pem_id)
                p_info["present"] = False

    def power_supply_probe(self) -> None:
        """
        Probe AC-PSU and DC-PEM power supply devices.
        """
        for p_id in self.power_supplies.keys():
            if p_id.startswith("psu"):
                self.psu_probe(p_id)
            elif p_id.startswith("pem"):
                self.pem_probe(p_id)

    def power_supply_init(self) -> None:
        """
        Initialize power supply devices.
        """
        detected_units = []
        for p_id, p_info in self.power_supplies.items():
            if p_info["present"]:
                detected_units.append(p_id)
        detected_units.sort()
        syslog(
            LOG_INFO,
            "total %d power unit(s) detected: %s"
            % (len(detected_units), ", ".join(detected_units)),
        )

        if self.ltc4281_info["present"]:
            self.ltc4281_init()
        elif self.ltc4151_info["present"]:
            self.ltc4151_init()

    def power_type_cache_init(self) -> None:
        """
        Store detected power supply devices in cache file.
        """
        if os.path.exists(POWER_TYPE_CACHE):
            run_shell_cmd("rm %s" % POWER_TYPE_CACHE)
            run_shell_cmd("touch %s" % POWER_TYPE_CACHE)

        # NOTE: we use pem1 to refer to ltc4151 and pem2 for ltc4281 in
        # the cache file, and this is different from psu1/psu2, whose
        # index specifies slot/channel where the psu is plugged.
        for p_id, p_info in self.power_supplies.items():
            value = None

            if p_id.startswith("psu"):
                value = int(p_info["present"])
            elif p_id == "pem1":
                value = int(self.ltc4151_info["present"])
            elif p_id == "pem2":
                value = int(self.ltc4281_info["present"])

            if isinstance(value, int):
                run_shell_cmd("echo %s: %d >> %s" % (p_id, value, POWER_TYPE_CACHE))

    def power_consumption_reduction(self) -> None:
        """
        Power off userver and switching ASIC, and reset QSFP.
        We will only reduce power is userver is already on. This will
        ensure that we don't end up preventing a power sequence control
        from kicking in which could cause 7-0070 not to be detectable.
        """
        if self.userver_power_is_on():
            syslog(LOG_CRIT, "Power consumption reduction start")

            syslog(LOG_CRIT, "Power off userver")
            syscpld_write_int("pwr_usrv_en", 0)
            # Wait for a while to make sure command execute done
            time.sleep(1)

            syslog(LOG_CRIT, "Power off switching ASIC")
            syscpld_write_int("pwr_main_n", 0)
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
            syslog(LOG_CRIT, "Power consumption reduction finish")
        else:
            syslog(LOG_CRIT, "userver already off: skip power reduction.")

    def userver_power_is_on(self) -> bool:
        """
        Detect both of userver power enable(syscpld register 0x30 bit 2)
        and main power enable(syscpld register 0x30 bit 1)
        """
        userver = syscpld_read_int("pwr_usrv_en")
        main = syscpld_read_int("pwr_main_n")

        if isinstance(userver, int) and isinstance(main, int):
            val = userver | main
            if val == 1:
                return True

        return False

    def ltc4151_init(self) -> None:
        """
        Initialize ltc4151.
        """
        syslog(LOG_INFO, "initializing ltc4151")

        path = os.path.join(self.ltc4151_info["sysfs_path"], "hwmon")
        for entry in os.listdir(path):
            if entry.startswith("hwmon"):
                hwmon_path = os.path.join(path, entry)
                self.ltc4151_info["hwmon_path"] = hwmon_path
                syslog(LOG_INFO, "set ltc4151 hwmon_path to %s" % hwmon_path)
                return

        syslog(LOG_ERR, "unable to locate ltc4151 hwmon path!")

    def ltc4151_read_vin(self) -> Union[int, None]:
        """
        Read LTC4151 reported voltage value.
        """
        try:
            hwmon_path = self.ltc4151_info["hwmon_path"]
            return sysfs_read_int(os.path.join(hwmon_path, LTC4151_HWMON_VIN))
        except Exception:
            return None

    def ltc4151_force_read_vin(self) -> Union[int, None]:
        """
        Try to read from ltc4151 until the device returns data.
        NOTE: it means the function may never return is the device is dead.
        """
        while True:
            vin = self.ltc4151_read_vin()
            if vin:
                return vin

            self.exec_env_prepare()
            time.sleep(2)

    def ltc4151_check_voltage(self) -> None:
        """
        check if the voltage is below the threshold. if
        if is <= 10V, we will reduce the load. However,
        if it is between 10 and 11v, we will compute the
        moving avg of 10 sample. Then we will check to see
        if it is below the 11v so that we can reduce the load
        """
        vin = self.ltc4151_force_read_vin()
        if vin < LTC4151_VIN_MIN_THRESHOLD:
            if vin < LTC4151_VIN_CRIT_THRESHOLD:
                syslog(
                    LOG_ERR,
                    "ltc4151: vin %d below crit threshold %d mV"
                    % (vin, LTC4151_VIN_CRIT_THRESHOLD),
                )
                crit_flag = True
            else:
                syslog(
                    LOG_ERR,
                    "ltc4151: vin %d below min threshold %d mV"
                    % (vin, LTC4151_VIN_MIN_THRESHOLD),
                )
                crit_flag = False

            # Read vin for 10 times and get average voltage.
            i = 0
            total_vin = 0
            while i < self.ltc4151_info["average_cnt"]:
                vin = self.ltc4151_force_read_vin()

                # if this reading is below 11V and the previous one was <= 10.5,
                # we will reduce the load immediately and then return.
                if vin < LTC4151_VIN_MIN_THRESHOLD and crit_flag:
                    syslog(LOG_CRIT, "ltc4151: vin below threshold consistently!")
                    self.power_consumption_reduction()
                    return

                if vin < LTC4151_VIN_CRIT_THRESHOLD:
                    crit_flag = True
                else:
                    crit_flag = False

                total_vin += vin
                i += 1

            average_vin = total_vin / self.ltc4151_info["average_cnt"]
            if average_vin < LTC4151_VIN_MIN_THRESHOLD:
                syslog(
                    LOG_CRIT,
                    "ltc4151: average vin %d below threshold %d"
                    % (average_vin, LTC4151_VIN_MIN_THRESHOLD),
                )
                self.power_consumption_reduction()

    def ltc4281_read_int(self, entry) -> Union[int, None]:
        """
        Read integer from the given ltc4281 sysfs entry.
        """
        hwmon_path = self.ltc4281_info["hwmon_path"]
        return sysfs_read_int(os.path.join(hwmon_path, entry))

    def ltc4281_write_int(self, entry, val) -> None:
        """
        Write value to the given ltc4281 sysfs entry.
        """
        hwmon_path = self.ltc4281_info["hwmon_path"]
        sysfs_write_int(os.path.join(hwmon_path, entry), val)

    def ltc4281_init(self, delay=0.05) -> None:
        """
        Initialize LTC4281.
        """
        syslog(LOG_INFO, "initializing ltc4281")

        self.ltc4281_info["hwmon_path"] = self.ltc4281_info["sysfs_path"]

        if not self.ltc4281_check_status():
            syslog(LOG_ERR, "ltc4281: fault status detected")
            self.ltc4281_dump_status()
            self.ltc4281_clear_status()

        self.ltc4281_write_int(LTC4281_FAULT_LOG_EN, 1)
        # Wait for a while to make sure command execute done
        time.sleep(delay)
        # Clear this bit to default, because it will be set to 1 when we do
        # clear_ltc4281_status function (triggered by EEPROM_DONE_ALERT).
        self.ltc4281_write_int(LTC4281_ALERT_GENERATED, 0)

    def ltc4281_temp_fail_assert(self) -> None:
        """
        Save assert log caused by temperature reading fail
        """
        self.ltc4281_info["temp_read_error_cnt"] += 1
        if self.ltc4281_info["temp_read_error_cnt"] >= 10:
            self.ltc4281_info["temp_read_error_alarm"] = True
            syslog(LOG_CRIT, "ASSERT: ltc4281 temp alarm raised: no input!")

            if self.userver_power_is_on():
                self.power_consumption_reduction()

    def ltc4281_temp_fail_deassert(self) -> None:
        """
        Save deassert log for temperature is back to read
        """
        if self.ltc4281_info["temp_read_error_alarm"]:
            self.ltc4281_info["temp_read_error_cnt"] = 0
            self.ltc4281_info["temp_read_error_alarm"] = False
            syslog(LOG_INFO, "ltc4281: temp_no_input alarm cleared.")

    def ltc4281_temp_check_threshold(self, temp) -> None:
        """
        Temperature >= 85, record assert log,
        temperature is from >= 85 to < 85, record deassert log.
        Temperature >= 100, reduce power comsumption and record assert log,
        temperature is from >= 100 to < 100, record deassert log.
        """
        if temp >= TEMP_THRESHOLD_UNR:
            if not self.ltc4281_info["temp_unrecoverable_alarm"]:
                syslog(
                    LOG_CRIT,
                    "ltc4281: %s alarm raised: %.2f > %.2f C"
                    % (UNR_ALARM_NAME, temp, TEMP_THRESHOLD_UNR),
                )
                self.ltc4281_info["temp_unrecoverable_alarm"] = True
                self.ltc4281_dump_status()
                self.power_consumption_reduction()
        elif temp < TEMP_THRESHOLD_UNR and temp >= TEMP_THRESHOLD_UCR:
            if self.ltc4281_info["temp_unrecoverable_alarm"]:
                self.ltc4281_info["temp_unrecoverable_alarm"] = False
                syslog(
                    LOG_INFO,
                    "ltc4281: %s alarm settled, temp %.2f C" % (UNR_ALARM_NAME, temp),
                )

            if not self.ltc4281_info["temp_upper_crit_alarm"]:
                syslog(
                    LOG_ERR,
                    "ltc4281: %s alarm raised: MOSFET temp %.2f > %.2f C"
                    % (UCR_ALARM_NAME, temp, TEMP_THRESHOLD_UCR),
                )
                self.ltc4281_info["temp_upper_crit_alarm"] = True
                self.ltc4281_dump_status()
        else:
            if self.ltc4281_info["temp_unrecoverable_alarm"]:
                self.ltc4281_info["temp_unrecoverable_alarm"] = False
                syslog(
                    LOG_INFO,
                    "ltc4281: %s alarm settled, temp %.2f C" % (UNR_ALARM_NAME, temp),
                )
                self.ltc4281_dump_status()

            if self.ltc4281_info["temp_upper_crit_alarm"]:
                self.ltc4281_info["temp_upper_crit_alarm"] = False
                syslog(
                    LOG_INFO,
                    "ltc4281: %s alarm settled, temp %.2f C" % (UCR_ALARM_NAME, temp),
                )
                self.ltc4281_dump_status()

    def ltc4281_check_temp(self) -> None:
        """
        Check temperature value is valid or not,
        if value is valid, check it is over threshold or not.
        """
        temp = self.ltc4281_read_int(LTC4281_TEMP)

        if temp is None:
            self.ltc4281_temp_fail_assert()
        else:
            self.ltc4281_temp_fail_deassert()
            temp = temp / 1000
            self.ltc4281_temp_check_threshold(temp)

    def ltc4281_check_fet(self) -> None:
        """
        If one of MOSFET status is bad,
        reduce power comsumption and record assert log.
        All MOSFET status is normal now and have occoured MOSFET bad before,
        record deassert log.
        """
        fet_short = self.ltc4281_read_int("fet_short_present")
        fet_bad_cool = self.ltc4281_read_int("fet_bad_cooldown_status")
        fet_bad_fault = self.ltc4281_read_int("fet_bad_fault")
        fet_bad_fault_ee = self.ltc4281_read_int("fet_bad_fault_ee")

        if (
            isinstance(fet_short, int)
            and isinstance(fet_bad_cool, int)
            and isinstance(fet_bad_fault, int)
            and isinstance(fet_bad_fault_ee, int)
        ):
            val = fet_short | fet_bad_cool | fet_bad_fault | fet_bad_fault_ee
            if val == 1:
                self.ltc4281_info["fet_bad_cnt"] += 1
                if self.ltc4281_info["fet_bad_cnt"] >= 5:
                    if not self.ltc4281_info["fet_bad_alarm"]:
                        self.ltc4281_info["fet_bad_alarm"] = True

                        syslog(LOG_CRIT, "MOSFET bad status alarm raised")
                        self.ltc4281_dump_status()
                        self.power_consumption_reduction()
            else:
                self.ltc4281_info["fet_bad_cnt"] = 0
                if self.ltc4281_info["fet_bad_alarm"]:
                    self.ltc4281_info["fet_bad_alarm"] = False
                    syslog(LOG_CRIT, "MOSFET bad status alarm settled")
                    self.ltc4281_dump_status()

    def ltc4281_check_status(self, delay=0.01) -> bool:
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
            val = self.ltc4281_read_int(status)
            if isinstance(val, int) and val == 1:
                return False
            # Wait for a while to release cpu usage
            time.sleep(delay)

        for index in range(len(LTC4281_STATUS1)):
            val = self.ltc4281_read_int(LTC4281_STATUS1[index])
            if index == 3 or index == 4 or index == 7:
                if isinstance(val, int) and val == 0:
                    return False
            else:
                if isinstance(val, int) and val == 1:
                    return False
            # Wait for a while to release cpu usage
            time.sleep(delay)

        for index in range(len(LTC4281_STATUS2)):
            val = self.ltc4281_read_int(LTC4281_STATUS2[index])
            if index == 4 or index == 5 or index == 6:
                if isinstance(val, int) and val == 0:
                    return False
            else:
                if isinstance(val, int) and val == 1:
                    return False
            # Wait for a while to release cpu usage
            time.sleep(delay)

        return True

    def ltc4281_dump_status(self, delay=0.01) -> None:
        """
        Dump source voltage, drain voltage, GPIO3 voltage, current and power.
        Dump all LTC4281 status register
        (0x1e, 0x1f, 0x04, 0x05, 0x24, 0x25) value.
        """
        record = []
        for status in LTC4281_SENSOR:
            val = self.ltc4281_read_int(status[1])
            if isinstance(val, int):
                val = "%.2f" % (val / 1000) + " " + status[2]
            record.append((status[0] + ": " + val))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog(LOG_INFO, "ltc4281 sensors: %s" % record)

        record = []
        for status in LTC4281_STATUS1:
            val = self.ltc4281_read_int(status)
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog(LOG_INFO, "ltc4281 status1: %s at reg 0x1e" % record)

        record = []
        for status in LTC4281_STATUS2:
            val = self.ltc4281_read_int(status)
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog(LOG_INFO, "ltc4281 status2: %s at reg 0x1f" % record)

        record = []
        for status in LTC4281_FAULT_LOG:
            val = self.ltc4281_read_int(status)
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog(LOG_INFO, "ltc4281 fault log: %s at reg 0x04" % record)

        record = []
        for status in LTC4281_ADC_ALERT_LOG:
            val = self.ltc4281_read_int(status)
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog(LOG_INFO, "ltc4281 adc alert: %s at reg 0x05" % record)

        record = []
        for status in LTC4281_FAULT_LOG_EE:
            val = self.ltc4281_read_int(status)
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog(LOG_INFO, "ltc4281 fault log ee: %s at reg 0x24" % record)

        record = []
        for status in LTC4281_ADC_ALERT_LOG_EE:
            val = self.ltc4281_read_int(status)
            record.append((status + ": " + str(val)))
            # Wait for a while to release cpu usage
            time.sleep(delay)
        syslog(LOG_INFO, "ltc4281 adc alert log ee: %s at reg 0x25" % record)

    def ltc4281_clear_status(self, delay=0.05) -> None:
        """
        Clear LTC4281 status register (0x04, 0x05, 0x24, 0x25) value.
        """
        log_status = (
            LTC4281_FAULT_LOG_EE
            + LTC4281_ADC_ALERT_LOG_EE
            + LTC4281_FAULT_LOG
            + LTC4281_ADC_ALERT_LOG
        )
        for status in log_status:
            self.ltc4281_write_int(status, 0)
            # Wait for a while to make sure command execute done
            time.sleep(delay)

    def i2c_mux_get_ready(self) -> bool:
        """
        Manually bind pca954x driver to i2c mux 7-0070 if needed.
        """
        path = os.path.join(SYS_I2C_DRIVERS, I2C_MUX_DRIVER, I2C_MUX_DEVICE)
        if not os.path.exists(path):
            i2c_driver_bind(I2C_MUX_DRIVER, I2C_MUX_DEVICE)
            time.sleep(2)

        bus14_path = os.path.join(SYS_I2C_DEVICES, "i2c-14")
        bus15_path = os.path.join(SYS_I2C_DEVICES, "i2c-15")
        return os.path.exists(bus14_path) and os.path.exists(bus15_path)

    def exec_env_prepare(self) -> None:
        """
        Make sure uServer is powered, and i2c-mux 7-0070 is live; otherwise,
        neigher PSU nor PEM can be accessed.
        """
        while True:
            if self.userver_power_is_on():
                if not self.userver_is_powered:
                    syslog(LOG_INFO, "userver switched to power-on state.")
                    self.userver_is_powered = True

                if self.i2c_mux_get_ready():
                    break
            else:
                if self.userver_is_powered:
                    syslog(LOG_ERR, "userver lost power!")
                    self.userver_is_powered = False

            time.sleep(5)

    def run(self) -> None:
        """
        Main psumuxmon method
        """

        self.exec_env_prepare()
        time.sleep(5)

        self.power_supply_probe()

        # PSU/PEM devices were created in power_supply_probe() but driver
        # binding may be delayed a bit. Let's sleep for a short period so
        # the drivers are ready.
        time.sleep(5)

        self.power_supply_init()

        self.power_type_cache_init()

        if not self.ltc4281_info["present"] and not self.ltc4151_info["present"]:
            syslog(LOG_INFO, "no pem device detected. Exiting.")
            sys.exit(0)

        while True:
            self.exec_env_prepare()

            if self.ltc4281_info["present"]:
                self.ltc4281_check_temp()
                self.ltc4281_check_fet()
            elif self.ltc4151_info["present"]:
                self.ltc4151_check_voltage()

            # Every 5s polling once
            time.sleep(5)


if __name__ == "__main__":
    openlog("psumuxmon")
    pcard = Pcard()
    pcard.run()
