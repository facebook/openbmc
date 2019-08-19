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

LTC4151_VIN_HWMON = LTC4151_PATH + "in1_input"
LTC4281_TEMP = LTC4281_PATH + "temp1_input"
LTC4281_FAULT_LOG_EN = LTC4281_PATH + "fault_log_enable"
LTC4281_ON_FAULT_MASK = LTC4281_PATH + "on_fault_mask"
LTC4281_ON_FAULT_MASK_EE = LTC4281_PATH + "on_fault_mask_ee"
USERVER_POWER = SYSCPLD_PATH + "pwr_usrv_en"
MAIN_POWER = SYSCPLD_PATH + "pwr_main_n"
PSU_PRESENT = {0x1: SYSCPLD_PATH + "psu2_present", 0x2: SYSCPLD_PATH + "psu1_present"}

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
        self.ltc4151_vin = None
        self.ltc4151_pcard_channel = None
        self.ltc4151_check_interval = 600
        self.ltc4151_check_cnt = 600
        self.ltc4281_ucr_assert = {0x1: False, 0x2: False}
        self.ltc4281_unr_assert = {0x1: False, 0x2: False}
        self.ltc4281_fet_bad = {0x1: False, 0x2: False}
        self.ltc4281_temp_no_val_cnt = {0x1: 0, 0x2: 0}
        self.ltc4281_temp_no_val = {0x1: False, 0x2: False}
        self.ltc4281_temp_no_val_power_off = {0x1: False, 0x2: False}
        self.first_detect = {0x1: True, 0x2: True}
        self.with_ltc4281 = {0x1: False, 0x2: False}
        self.present_status = {0x1: 1, 0x2: 1}
        self.present_channel = []

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
                    if val[ch] is 0:
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
                self.present_status[ch] = val[ch]
            else:
                self.first_detect[ch] = True
                syslog.syslog(
                    syslog.LOG_CRIT, "Mux channel %d pcard/psu extraction" % ch
                )
                self.present_status[ch] = 1

    def userver_power_is_on(self) -> bool:
        """
        Detect both of userver power enable(syscpld register 0x30 bit 2)
        and main power enable(syscpld register 0x30 bit 1)
        """
        userver = sysfs_read(USERVER_POWER)
        main = sysfs_read(MAIN_POWER)

        if isinstance(userver, int) and isinstance(main, int):
            val = userver | main
            if val is 1:
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

    def ltc4151_read(self, inp) -> Union[int, None]:
        """
        Read LTC4151 register value.
        """
        try:
            # After BMC comes up this hwmon device is setup once,
            # and we can cache that data for the first time
            # instead of determining the source each time.
            if not inp:
                inp = self.get_ltc4151_hwmon_source()
            return sysfs_read(inp)
        except Exception:
            return None

    def ltc4151_read_channel(self, ch, inp) -> Union[int, None]:
        """
        Switch mux channel and read LTC4151 register value.
        """
        set_mux_channel(ch)
        return self.ltc4151_read(inp)

    def ltc4281_detect(self, ch) -> None:
        """
        Switch mux channel and read PEM fru eeprom Product Version,
        if value = 1, means PEM without LTC4281,
        if value >= 2, means PEM with LTC4281.
        """
        if ch is 0x1:
            # PSU slot1 Fru eeprom address
            eeprom_addr = 0x56
        else:
            # PSU slot2 Fru eeprom address
            eeprom_addr = 0x55

        set_mux_channel(ch)
        exec_check_return(I2C_SET_BYTE % (MUX_BUS, eeprom_addr, 0x00))
        # Wait for a while to make sure command execute done
        time.sleep(0.05)
        set_mux_channel(ch)
        exec_check_return(I2C_SET_BYTE % (MUX_BUS, eeprom_addr, 0x4D))
        # Wait for a while to make sure command execute done
        time.sleep(0.05)
        val = exec_check_output(I2C_GET_BYTE % (MUX_BUS, eeprom_addr))
        if val == -1:
            syslog.syslog(syslog.LOG_CRIT, "Mux channel %d is ac psu" % ch)
            self.with_ltc4281[ch] = False
        elif val == 1:
            syslog.syslog(
                syslog.LOG_CRIT,
                "Mux channel %d is pcard without LTC4281(type=%d)" % (ch, val),
            )
            self.with_ltc4281[ch] = False
        elif val >= 2:
            syslog.syslog(
                syslog.LOG_CRIT,
                "Mux channel %d is pcard with LTC4281(type=%d)" % (ch, val),
            )
            self.dump_ltc4281_status(ch)
            self.clear_ltc4281_status(ch)
            self.init_ltc4281(ch)
            self.with_ltc4281[ch] = True
        else:
            syslog.syslog(
                syslog.LOG_CRIT, "Mux channel %d is unknown device(type=%d)" % (ch, val)
            )
            self.with_ltc4281[ch] = False

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
                    self.ltc4281_temp_no_val[ch] = True
            else:
                if not self.ltc4281_temp_no_val_power_off[ch]:
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
            if val is 1:
                if not self.ltc4281_fet_bad[ch]:
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

    def check_ltc4151_vol(self) -> None:
        """
        Check LTC4151 voltage value
        """
        live_channels = []
        for ch in self.present_channel:
            vin = self.ltc4151_read_channel(ch, self.ltc4151_vin)
            if vin:
                live_channels.append(ch)
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
            LTC4281_FAULT_LOG
            + LTC4281_ADC_ALERT_LOG
            + LTC4281_FAULT_LOG_EE
            + LTC4281_ADC_ALERT_LOG_EE
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
        syslog.syslog(
            syslog.LOG_CRIT, "Mux channel %d LTC4281 initialization finish" % ch
        )

    def run(self) -> None:
        """
        Main psumuxmon method
        """
        while True:
            self.present_detect()
            # Every 600s check LTC4151 voltage once
            if self.ltc4151_check_cnt >= self.ltc4151_check_interval:
                self.check_ltc4151_vol()
                self.ltc4151_check_cnt = 0
            else:
                self.ltc4151_check_cnt += 5

            for ch in self.present_channel:
                if self.first_detect[ch]:
                    if self.userver_power_is_on():
                        self.ltc4281_detect(ch)
                        self.first_detect[ch] = False

                if self.with_ltc4281[ch]:
                    self.check_ltc4281_temp(ch)
                    self.check_ltc4281_fet(ch)
            # Every 5s polling once
            time.sleep(5)


if __name__ == "__main__":
    syslog.openlog("psumuxmon")
    pcard = Pcard()
    pcard.run()
