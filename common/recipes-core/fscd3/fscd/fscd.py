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
import json
import os.path
import signal
import subprocess
import sys
import time
import traceback
import datetime

import fsc_expr
from fsc_bmcmachine import BMCMachine
from fsc_board import board_callout, board_fan_actions, board_host_actions
from fsc_profile import Sensor, profile_constructor
from fsc_sensor import FscSensorSourceUtil
from fsc_util import Logger, clamp
from fsc_zone import Fan, Zone, fan_mode


RECORD_DIR = "/tmp/cache_store/"
SENSOR_FAIL_RECORD_DIR = "/tmp/sensorfail_record/"
FAN_FAIL_RECORD_DIR = "/tmp/fanfail_record/"
RAMFS_CONFIG = "/etc/fsc-config.json"
CONFIG_DIR = "/etc/fsc"
# Enable the following for testing only
# RAMFS_CONFIG = '/tmp/fsc-config.json'
# CONFIG_DIR = '/tmp'
DEFAULT_INIT_BOOST = 100
DEFAULT_INIT_TRANSITIONAL = 70
WDTCLI_CMD = "/usr/local/bin/wdtcli"


def kick_watchdog():
    """kick the watchdog device.
    """
    f = subprocess.Popen(
        WDTCLI_CMD + " kick", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    info, err = f.communicate()
    if len(err) != 0:
        Logger.error("failed to kick watchdog device")


def stop_watchdog():
    """kick the watchdog device.
    """
    f = subprocess.Popen(
        WDTCLI_CMD + " stop", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    info, err = f.communicate()
    if len(err) != 0:
        Logger.error("failed to stop watchdog device")
    else:
        Logger.info("watchdog stopped")


class Fscd(object):

    DEFAULT_BOOST = 100
    DEFAULT_BOOST_TYPE = "default"
    DEFAULT_TRANSITIONAL = 70
    DEFAULT_RAMP_RATE = 10

    def __init__(
        self, config=RAMFS_CONFIG, zone_config=CONFIG_DIR, log_level="warning"
    ):
        Logger.start("fscd", log_level)
        Logger.info("Starting fscd")
        self.zone_config = zone_config
        self.fsc_config = self.get_fsc_config(config)  # json dump from config
        self.boost = self.DEFAULT_BOOST
        self.non_fanfail_limited_boost = None
        self.boost_type = self.DEFAULT_BOOST_TYPE
        self.transitional = self.DEFAULT_TRANSITIONAL
        self.ramp_rate = self.DEFAULT_RAMP_RATE
        self.sensor_fail = None
        self.ssd_progressive_algorithm = None
        self.sensor_valid_check = None
        self.fail_sensor_type = None
        self.fan_dead_boost = None
        self.fan_fail = None
        self.fan_recovery_pending = False
        self.fan_recovery_time = None

    # TODO: Add checks for invalid config file path
    def get_fsc_config(self, fsc_config):
        if os.path.isfile(fsc_config):
            Logger.info("Started, reading configuration from %s" % (fsc_config))
            with open(fsc_config, "r") as f:
                return json.load(f)

    def get_config_params(self):
        self.transitional = self.fsc_config["pwm_transition_value"]
        self.boost = self.fsc_config["pwm_boost_value"]
        if "non_fanfail_limited_boost_value" in self.fsc_config:
            self.non_fanfail_limited_boost = self.fsc_config[
                "non_fanfail_limited_boost_value"
            ]
        if "boost" in self.fsc_config and "fan_fail" in self.fsc_config["boost"]:
            self.fan_fail = self.fsc_config["boost"]["fan_fail"]
        if "boost" in self.fsc_config and "progressive" in self.fsc_config["boost"]:
            if self.fsc_config["boost"]["progressive"]:
                self.boost_type = "progressive"
        if "fan_dead_boost" in self.fsc_config:
            self.fan_dead_boost = self.fsc_config["fan_dead_boost"]
            self.all_fan_fail_counter = 0
        if "boost" in self.fsc_config and "sensor_fail" in self.fsc_config["boost"]:
            self.sensor_fail = self.fsc_config["boost"]["sensor_fail"]
            if self.sensor_fail:
                if "fail_sensor_type" in self.fsc_config:
                    self.fail_sensor_type = self.fsc_config["fail_sensor_type"]
                if "ssd_progressive_algorithm" in self.fsc_config:
                    self.ssd_progressive_algorithm = self.fsc_config[
                        "ssd_progressive_algorithm"
                    ]
        if "sensor_valid_check" in self.fsc_config:
            self.sensor_valid_check = self.fsc_config["sensor_valid_check"]
        self.watchdog = self.fsc_config["watchdog"]
        if "fanpower" in self.fsc_config:
            self.fanpower = self.fsc_config["fanpower"]
        else:
            self.fanpower = False
        if "chassis_intrusion" in self.fsc_config:
            self.chassis_intrusion = self.fsc_config["chassis_intrusion"]
        else:
            self.chassis_intrusion = False
        if "enable_fsc_sensor_check" in self.fsc_config:
            self.enable_fsc_sensor_check = self.fsc_config["enable_fsc_sensor_check"]
        else:
            self.enable_fsc_sensor_check = False
        if "ramp_rate" in self.fsc_config:
            self.ramp_rate = self.fsc_config["ramp_rate"]
        if self.watchdog:
            Logger.info("watchdog pinging enabled")
            kick_watchdog()
        self.interval = self.fsc_config["sample_interval_ms"] / 1000.0
        if "fan_recovery_time" in self.fsc_config:
            self.fan_recovery_time = self.fsc_config["fan_recovery_time"]

    def build_profiles(self):
        self.sensors = {}
        self.profiles = {}

        for name, pdata in list(self.fsc_config["profiles"].items()):
            self.sensors[name] = Sensor(name, pdata)
            self.profiles[name] = profile_constructor(pdata)

    def build_fans(self):
        self.fans = {}
        for name, pdata in list(self.fsc_config["fans"].items()):
            self.fans[name] = Fan(name, pdata)

    def build_zones(self):
        self.zones = []
        counter = 0
        for name, data in list(self.fsc_config["zones"].items()):
            filename = data["expr_file"]
            with open(os.path.join(self.zone_config, filename), "r") as exf:
                source = exf.read()
                Logger.info("Compiling FSC expression for zone:")
                Logger.info(source)
                (expr, inf) = fsc_expr.make_eval_tree(source, self.profiles)
                for name in inf["ext_vars"]:
                    sdata = name.split(":")
                    board = sdata[0]
                    # sname never used. so comment out (avoid lint error)
                    # sname = sdata[1]
                    if board not in self.machine.frus:
                        self.machine.nums[board] = []
                    self.machine.frus.add(board)
                    if len(sdata) == 3:
                        self.machine.nums[board].append(sdata[2])

                zone = Zone(
                    data["pwm_output"],
                    expr,
                    inf,
                    self.transitional,
                    counter,
                    self.boost,
                    self.sensor_fail,
                    self.sensor_valid_check,
                    self.fail_sensor_type,
                    self.ssd_progressive_algorithm,
                )
                counter += 1
                self.zones.append(zone)

    def build_machine(self):
        self.machine = BMCMachine()

    def fsc_fan_action(self, fan, action):
        """
        Method invokes board actions for a fan.
        """
        if "dead" in action:
            board_fan_actions(fan, action="dead")
            board_fan_actions(fan, action="led_red")
        if "recover" in action:
            board_fan_actions(fan, action="recover")
            board_fan_actions(fan, action="led_blue")

    def fsc_host_action(self, action, cause):
        if "host_shutdown" in action:
            return board_host_actions(action="host_shutdown", cause=cause)
            # board_fan_actions(fan, action='led_blue')

    def fsc_set_all_fan_led(self, color):
        for fan, _value in list(self.fans.items()):
            board_fan_actions(self.fans[fan], action=color)

    def fsc_safe_guards(self, sensors_tuples):
        """
        Method defines safe guards for fsc.
        Examples: Triggers board action when sensor temp read reaches limits
        configured in json
        """
        for fru in self.machine.frus:
            for sensor, tuple in list(sensors_tuples[fru].items()):
                if tuple.name in self.fsc_config["profiles"]:
                    if "read_limit" in self.fsc_config["profiles"][tuple.name]:
                        # If temperature read exceeds accpetable temperature reading
                        if (
                            "valid"
                            in self.fsc_config["profiles"][tuple.name]["read_limit"]
                        ):
                            valid_table = self.fsc_config["profiles"][tuple.name][
                                "read_limit"
                            ]["valid"]
                            valid_read_limit = valid_table["limit"]
                            valid_read_action = valid_table["action"]
                            valid_read_th = valid_table["threshold"]
                            if isinstance(
                                self.sensors[tuple.name].source, FscSensorSourceUtil
                            ):
                                if tuple.value == None:
                                    self.sensors[
                                        tuple.name
                                    ].source.read_source_fail_counter += 1
                                else:
                                    self.sensors[
                                        tuple.name
                                    ].source.read_source_fail_counter = 0
                                    if tuple.value > valid_read_limit:
                                        reason = (
                                            sensor
                                            + "(v="
                                            + str(tuple.value)
                                            + ") limit(t="
                                            + str(valid_read_limit)
                                            + ") reached"
                                        )
                                        self.fsc_host_action(
                                            action=valid_read_action, cause=reason
                                        )
                            else:
                                if tuple.value > valid_read_limit:
                                    if tuple.wrong_read_counter < valid_read_th:
                                        self.sensors[
                                            tuple.name
                                        ].source.read_source_wrong_counter += 1
                                        Logger.warn(
                                            "inlet_temp v=%d, and counter=%d"
                                            % (tuple.value, tuple.wrong_read_counter)
                                        )
                                        continue
                                    reason = (
                                        sensor
                                        + "(v="
                                        + str(tuple.value)
                                        + ") limit(t="
                                        + str(valid_read_limit)
                                        + ") reached"
                                    )
                                    self.fsc_host_action(
                                        action=valid_read_action, cause=reason
                                    )
                                self.sensors[
                                    tuple.name
                                ].source.read_source_wrong_counter = 0

                        # If temperature read fails
                        if (
                            "invalid"
                            in self.fsc_config["profiles"][tuple.name]["read_limit"]
                        ):
                            invalid_table = self.fsc_config["profiles"][tuple.name][
                                "read_limit"
                            ]["invalid"]
                            invalid_read_th = invalid_table["threshold"]
                            invalid_read_action = invalid_table["action"]
                            if isinstance(
                                self.sensors[tuple.name].source, FscSensorSourceUtil
                            ):
                                read_fail_counter = self.sensors[
                                    tuple.name
                                ].source.read_source_fail_counter
                                if read_fail_counter >= invalid_read_th:
                                    reason = (
                                        sensor
                                        + "(value="
                                        + str(tuple.value)
                                        + ") failed to read "
                                        + str(read_fail_counter)
                                        + " times"
                                    )
                                    self.fsc_host_action(
                                        action=invalid_read_action, cause=reason
                                    )
                            else:
                                if tuple.read_fail_counter >= invalid_read_th:
                                    reason = (
                                        sensor
                                        + "(value="
                                        + str(tuple.value)
                                        + ") failed to read "
                                        + str(tuple.read_fail_counter)
                                        + " times"
                                    )
                                    self.fsc_host_action(
                                        action=invalid_read_action, cause=reason
                                    )

    def fsc_sensor_check(self, sensors_tuples):
        """
            Monitor sensor temperature value
            This function checks whether any thermal sensor temp is over limit or not
            if it over limit it will boost the fan to full speed

            return 0 is normal
                   1 is sensor(s) valus is violate
        """
        ret = 0
        for fru in self.machine.frus:
            for sensor, tuple in list(sensors_tuples[fru].items()):
                if tuple.name in self.fsc_config["profiles"]:
                    last_error_time = self.sensors[tuple.name].source.last_error_time
                    last_error_level = self.sensors[tuple.name].source.last_error_level
                    if "read_limit" in self.fsc_config["profiles"][tuple.name]:
                        # If temperature read exceeds accpetable temperature reading
                        if (
                            "alarm_major"
                            in self.fsc_config["profiles"][tuple.name]["read_limit"]
                        ):
                            valid_table = self.fsc_config["profiles"][tuple.name][
                                "read_limit"
                            ]["alarm_major"]
                            valid_read_limit = valid_table["limit"]
                            if tuple.value > valid_read_limit:
                                reason = (
                                    sensor
                                    + "(alarm_major v="
                                    + str(tuple.value)
                                    + ") limit(t="
                                    + str(valid_read_limit)
                                    + ") reached"
                                )
                                last_error_level = "alarm_major"
                                self.sensors[tuple.name].source.last_error_time = int(
                                    datetime.datetime.now().strftime("%s")
                                )
                                if "action" in valid_table:
                                    self.fsc_host_action(
                                        action=valid_table["action"], cause=reason
                                    )
                                Logger.warn(reason)
                                ret = 1
                            elif (
                                "soak_time_s" in valid_table
                                and last_error_level is "alarm_major"
                            ):
                                elapsed_time = (
                                    int(datetime.datetime.now().strftime("%s"))
                                    - last_error_time
                                )
                                if elapsed_time < valid_table["soak_time_s"]:
                                    reason = (
                                        sensor
                                        + "(alarm_major elapsed_time = "
                                        + str(elapsed_time)
                                        + ",soak_time = "
                                        + str(valid_table["soak_time_s"])
                                        + ") reached"
                                    )
                                    if "action" in valid_table:
                                        self.fsc_host_action(
                                            action=valid_table["action"], cause=reason
                                        )
                                    Logger.warn(reason)
                                    ret = 1
                                else:
                                    last_error_level = None

                        if (
                            "alarm_minor"
                            in self.fsc_config["profiles"][tuple.name]["read_limit"]
                            and last_error_level != "alarm_major"
                        ):
                            valid_table = self.fsc_config["profiles"][tuple.name][
                                "read_limit"
                            ]["alarm_minor"]
                            if tuple.value > valid_read_limit:
                                reason = (
                                    sensor
                                    + "(alarm_minor v="
                                    + str(tuple.value)
                                    + ") limit(t="
                                    + str(valid_read_limit)
                                    + ") reached"
                                )
                                last_error_level = "alarm_minor"
                                self.sensors[tuple.name].source.last_error_time = int(
                                    datetime.datetime.now().strftime("%s")
                                )
                                if "action" in valid_table:
                                    self.fsc_host_action(
                                        action=valid_table["action"], cause=reason
                                    )
                                Logger.warn(reason)
                                ret = 1
                            elif (
                                "soak_time_s" in valid_table
                                and last_error_level == "alarm_minor"
                            ):
                                elapsed_time = (
                                    int(datetime.datetime.now().strftime("%s"))
                                    - last_error_time
                                )
                                if elapsed_time < valid_table["soak_time_s"]:
                                    reason = (
                                        sensor
                                        + "(alarm_minor elapsed_time = "
                                        + str(elapsed_time)
                                        + ",soak_time = "
                                        + str(valid_table["soak_time_s"])
                                        + ") reached"
                                    )
                                    if "action" in valid_table:
                                        self.fsc_host_action(
                                            action=valid_table["action"], cause=reason
                                        )
                                    Logger.warn(reason)
                                    ret = 1
                                else:
                                    last_error_level = None

                        self.sensors[
                            tuple.name
                        ].source.last_error_level = last_error_level
        return ret

    def update_dead_fans(self, dead_fans):
        """
        Check for dead and recovered fans
        """
        last_dead_fans = dead_fans.copy()
        speeds = self.machine.read_fans(self.fans)
        print("\x1b[2J\x1b[H")
        sys.stdout.flush()

        for fan, rpms in list(speeds.items()):
            Logger.info("%s speed: %d RPM" % (fan.label, rpms))
            if rpms < self.fsc_config["min_rpm"]:
                dead_fans.add(fan)
                self.fsc_fan_action(fan, action="dead")
            else:
                dead_fans.discard(fan)

        recovered_fans = last_dead_fans - dead_fans
        newly_dead_fans = dead_fans - last_dead_fans
        if len(newly_dead_fans) > 0:
            if self.fanpower:
                Logger.warn("%d fans failed" % (len(dead_fans),))
            else:
                Logger.crit("%d fans failed" % (len(dead_fans),))
            for dead_fan in dead_fans:
                if self.fanpower:
                    Logger.warn("%s dead, %d RPM" % (dead_fan.label, speeds[dead_fan]))
                else:
                    Logger.crit("%s dead, %d RPM" % (dead_fan.label, speeds[dead_fan]))
                Logger.usbdbg("%s fail" % (dead_fan.label))
                fan_fail_record_path = FAN_FAIL_RECORD_DIR + "%s" % (dead_fan.label)
                if not os.path.isfile(fan_fail_record_path):
                    fan_fail_record = open(fan_fail_record_path, "w")
                    fan_fail_record.close()
        for fan in recovered_fans:
            if self.fanpower:
                Logger.warn("%s has recovered" % (fan.label,))
            else:
                Logger.crit("%s has recovered" % (fan.label,))
            Logger.usbdbg("%s recovered" % (fan.label))
            self.fsc_fan_action(fan, action="recover")
            fan_fail_record_path = FAN_FAIL_RECORD_DIR + "%s" % (fan.label)
            if os.path.isfile(fan_fail_record_path):
                os.remove(fan_fail_record_path)
        return dead_fans

    def update_zones(self, dead_fans, time_difference):
        """
            TODO: Need to change logic here.

            # Platforms with chassis_intrusion mode enabled
            if chassis_intrusion:
                set the chassis_intrusion_boost_flag to 0
                and then do necessary checks to set flag to 1
                if chassis_intrusion_boost_flag:
                    run boost mode
                else:
                    run normal mode
            else
                # Platforms WITHOUT chassis_intrusion mode
                run normal mode

            # Platforms with enable_fsc_sensor_check mode enabled
            if enable_fsc_sensor_check:
                set the sensor_violated_flag to 0
                and then do necessary checks to set flag to 1
                if sensor_violated_flag:
                    run boost mode
                else:
                    run normal mode
            else
                # Platforms WITHOUT enable_fsc_sensor_check mode
                run normal mode

        """
        ctx = {}
        sensors_tuples = self.machine.read_sensors(self.sensors)
        self.fsc_safe_guards(sensors_tuples)
        for zone in self.zones:
            Logger.info("PWM: %s" % (json.dumps(zone.pwm_output)))
            mode = 0
            chassis_intrusion_boost_flag = 0
            sensor_violated_flag = 0
            if self.chassis_intrusion:
                self_tray_pull_out = board_callout(callout="chassis_intrusion")
                if self_tray_pull_out == 1:
                    chassis_intrusion_boost_flag = 1
            if self.enable_fsc_sensor_check:
                Logger.info("enable_fsc_sensor_check")
                if self.fsc_sensor_check(sensors_tuples) != 0:
                    sensor_violated_flag = 1

            if chassis_intrusion_boost_flag == 0 and sensor_violated_flag == 0:
                ctx["dt"] = time_difference
                ctx["dead_fans"] = dead_fans
                ignore_fan_mode = False
                if self.non_fanfail_limited_boost and dead_fans:
                    ignore_fan_mode = True
                pwmval = zone.run(
                    sensors=sensors_tuples, ctx=ctx, ignore_mode=ignore_fan_mode
                )
                mode = zone.get_set_fan_mode(mode, action="read")
            else:
                pwmval = self.boost
                mode = fan_mode["boost_mode"]

            if self.fan_fail:
                boost_record_path = RECORD_DIR + "fan_fail_boost"
                if self.boost_type == "progressive" and self.fan_dead_boost:
                    # Cases where we want to progressively bump PWMs
                    dead = len(dead_fans)
                    if dead > 0:
                        Logger.info(
                            "Progressive mode: Failed fans: %s"
                            % (", ".join([str(i.label) for i in dead_fans]))
                        )
                        for fan_count, rate in self.fan_dead_boost["data"]:
                            if dead <= fan_count:
                                pwmval = clamp(pwmval + (dead * rate), 0, 100)
                                mode = fan_mode["normal_mode"]
                                if os.path.isfile(boost_record_path):
                                    os.remove(boost_record_path)
                                break
                        else:
                            pwmval = self.boost
                            mode = fan_mode["boost_mode"]
                            if not os.path.isfile(boost_record_path):
                                fan_fail_boost_record = open(boost_record_path, "w")
                                fan_fail_boost_record.close()
                    else:
                        if os.path.isfile(boost_record_path):
                            os.remove(boost_record_path)
                else:
                    if dead_fans:
                        # If not progressive ,when there is 1 fan failed, boost all fans
                        Logger.info(
                            "Failed fans: %s"
                            % (", ".join([str(i.label) for i in dead_fans]))
                        )
                        pwmval = self.boost
                        mode = fan_mode["boost_mode"]
                        if not os.path.isfile(boost_record_path):
                            fan_fail_boost_record = open(boost_record_path, "w")
                            fan_fail_boost_record.close()
                    else:
                        if os.path.isfile(boost_record_path):
                            os.remove(boost_record_path)
                    if self.fan_dead_boost:
                        # If all the fans failed take action after a few cycles
                        if len(dead_fans) == len(self.fans):
                            self.all_fan_fail_counter = self.all_fan_fail_counter + 1
                            Logger.warn(
                                "Currently all fans failed for {} cycles".format(
                                    self.all_fan_fail_counter
                                )
                            )
                            if (
                                self.fan_dead_boost["threshold"]
                                and self.fan_dead_boost["action"]
                            ):
                                if (
                                    self.all_fan_fail_counter
                                    >= self.fan_dead_boost["threshold"]
                                ):
                                    self.fsc_host_action(
                                        action=self.fan_dead_boost["action"],
                                        cause="All fans are bad for more than "
                                        + str(self.fan_dead_boost["threshold"])
                                        + " cycles",
                                    )
                        else:
                            # If atleast 1 fan is working reset the counter
                            self.all_fan_fail_counter = 0

            # if no fan fail, the max of pwm is non_fanfail_limited_boost pwm:
            if self.non_fanfail_limited_boost and not dead_fans:
                pwmval = clamp(pwmval, 0, self.non_fanfail_limited_boost)

            if abs(zone.last_pwm - pwmval) > self.ramp_rate:
                if pwmval < zone.last_pwm:
                    pwmval = zone.last_pwm - self.ramp_rate
                else:
                    pwmval = zone.last_pwm + self.ramp_rate
            zone.last_pwm = pwmval

            if hasattr(zone.pwm_output, "__iter__"):
                for output in zone.pwm_output:
                    self.machine.set_pwm(self.fans.get(str(output)), pwmval)
            else:
                self.machine.set_pwm(self.fans[zone.pwm_output], pwmval)

            zone.get_set_fan_mode(mode, action="write")

    def builder(self):
        """
        Method to extract from json and build all internal data staructures
        """
        # Build a bmc machine object - read/write sensors
        self.build_machine()
        # Extract everything from json
        self.get_config_params()
        self.build_fans()
        self.build_profiles()
        Logger.info("Available profiles: " + ", ".join(list(self.profiles.keys())))
        self.build_zones()
        Logger.info("Read %d zones" % (len(self.zones)))
        Logger.info("Including sensors from: " + ", ".join(self.machine.frus))

    def get_fan_power_status(self):
        """
        Method invokes board action to determine fan power status.
        If not applicable returns True.
        """
        if board_callout(callout="read_power"):
            return True
        return False

    def fail_record_dir(self):
        """
        Create directory to store which sensors and fans failed
        """

        if not os.path.isdir(RECORD_DIR):
            os.mkdir(RECORD_DIR)

        if not os.path.isdir(SENSOR_FAIL_RECORD_DIR):
            os.mkdir(SENSOR_FAIL_RECORD_DIR)

        if not os.path.isdir(FAN_FAIL_RECORD_DIR):
            os.mkdir(FAN_FAIL_RECORD_DIR)

    def run(self):
        """
        Main FSCD method that builds from the fscd config and runs
        """

        # Get everything from json and build profiles, fans, zones
        self.builder()

        self.fail_record_dir()

        self.machine.set_all_pwm(self.fans, self.transitional)
        self.fsc_set_all_fan_led(color="led_blue")

        mode = fan_mode["trans_mode"]
        self.zones[0].get_set_fan_mode(mode, action="write")

        last = time.time()
        dead_fans = set()

        if self.fanpower:
            time.sleep(30)

        while True:
            if self.watchdog:
                kick_watchdog()

            time.sleep(self.interval)

            if self.fanpower:
                if not self.get_fan_power_status():
                    self.fan_recovery_pending = True
                    continue
            if self.fan_fail:
                if self.fan_recovery_pending and self.fan_recovery_time != None:
                    # Accelerating, wait for a while
                    time.sleep(self.fan_recovery_time)
                    self.fan_recovery_pending = False
                # Get dead fans for determining speed
                dead_fans = self.update_dead_fans(dead_fans)

            now = time.time()
            time_difference = now - last
            last = now

            # Check sensors and update zones
            self.update_zones(dead_fans, time_difference)


def handle_term(signum, frame):
    board_callout(callout="init_fans", boost=DEFAULT_INIT_TRANSITIONAL)
    Logger.warn("killed by signal %d" % (signum,))
    if signum == signal.SIGQUIT or signum == signal.SIGTERM or signum == signal.SIGINT:
        stop_watchdog()
    sys.exit("killed")


if __name__ == "__main__":
    try:
        signal.signal(signal.SIGTERM, handle_term)
        signal.signal(signal.SIGINT, handle_term)
        signal.signal(signal.SIGQUIT, handle_term)
        if len(sys.argv) > 1:
            llevel = sys.argv[1]
        else:
            llevel = "warning"
        fscd = Fscd(log_level=llevel)
        fscd.run()
    except Exception:
        board_callout(callout="init_fans", boost=DEFAULT_INIT_TRANSITIONAL)
        (etype, e) = sys.exc_info()[:2]
        Logger.crit("failed, exception: " + str(etype))
        traceback.print_exc()
        for line in traceback.format_exc().split("\n"):
            Logger.crit(line)
