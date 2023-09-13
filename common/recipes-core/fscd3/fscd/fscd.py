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
import ctypes
import datetime
import json
import os.path
import signal
import sys
import threading
import time
import traceback
from contextlib import contextmanager

import fsc_expr
import kv
from fsc_bmcmachine import BMCMachine
from fsc_board import board_callout, board_fan_actions, board_host_actions
from fsc_common_var import fan_mode
from fsc_profile import Sensor, profile_constructor
from fsc_sensor import FscSensorSourceUtil, FscSensorSourceJson
from fsc_util import Logger, clamp
from fsc_zone import Fan, Zone, BoardFanMode

try:
    libwatchdog = ctypes.CDLL("libwatchdog.so")
except OSError:
    # Sometimes libwatchdog is only available as libwatchdog.so.0
    libwatchdog = ctypes.CDLL("libwatchdog.so.0")

RECORD_DIR = "/tmp/cache_store/"
SENSOR_FAIL_RECORD_DIR = "/tmp/sensorfail_record/"
FAN_FAIL_RECORD_DIR = "/tmp/fanfail_record/"
RAMFS_CONFIG = "/etc/fsc-config.json"
CONFIG_DIR = "/etc/fsc"
FAN_DEAD_REARM_KEY = "fan_dead_rearm"
# Enable the following for testing only
# RAMFS_CONFIG = '/tmp/fsc-config.json'
# CONFIG_DIR = '/tmp'
DEFAULT_INIT_BOOST = 100
DEFAULT_INIT_TRANSITIONAL = 70


class LibWatchdogError(Exception):
    pass


def open_watchdog():
    """
    Open the watchdog. Once watchdog is opened by fscd, other processes
    won't be able to control watchdog until watchdog is closed by fscd.
    """
    if libwatchdog.open_watchdog(0, 0) != 0:
        raise LibWatchdogError("Failed to open watchdog")


def release_watchdog():
    """
    Close the watchdog opened by watchdog_open().
    """
    if libwatchdog.release_watchdog() != 0:
        raise LibWatchdogError("Failed to release watchdog")


def kick_watchdog():
    """kick the watchdog device."""
    if libwatchdog.kick_watchdog() != 0:
        raise LibWatchdogError("Failed to kick watchdog")


def stop_watchdog():
    """stop the watchdog timer."""
    ret = libwatchdog.stop_watchdog()
    if ret != 0:
        raise LibWatchdogError("stop_watchdog() returned " + str(ret))


def fscd_setup_watchdog():
    """
    Open the watchdog and start a thread to kick the watchdog periodically.
    """
    # Note: we will not release the watchdog until fscd exists, and it
    # also means other processes won't be able to control watchdog unless
    # killing fscd.
    open_watchdog()

    # An extra kicking is not necessary, but it doesn't hurt to do so.
    kick_watchdog()

    #
    # XXX is it a good idea to kick watchdog in a separate thread???
    # If fscd main thread fscd.run() got stuck, then BMC will be running
    # without thermal control, and watchdog cannot help us in this case.
    #
    _WATCHDOG_THREAD.start()


def fscd_release_watchdog(stop_wdt=False):
    """stop the watchdog device."""
    if _WATCHDOG_THREAD.is_alive():
        Logger.info("Stopping watchdog thread")
        _WATCHDOG_STOP.set()
        _WATCHDOG_THREAD.join()
        Logger.info("Watchdog thread stopped")

    if stop_wdt:
        stop_watchdog()
        Logger.info("watchdog stopped")

    release_watchdog()


## Watchdog thread definition, see {start,stop}_watchdog() above
def _watchdog_thread_f():
    while not _WATCHDOG_STOP.is_set():
        kick_watchdog()
        time.sleep(5)


_WATCHDOG_THREAD = threading.Thread(target=_watchdog_thread_f, daemon=True)
_WATCHDOG_STOP = threading.Event()


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
        self.fan_limit_upper_pwm = None
        self.fan_limit_lower_pwm = None
        self.sensor_filter_all = False
        self.sensor_fail_ignore = False
        self.pwm_sensor_boost_value = None
        self.output_max_boost_pwm = False
        self.board_fan_mode = BoardFanMode()
        self.need_rearm = False
        self.multi_fan_fail = None
        self.standby_fan_fail = None

    # TODO: Add checks for invalid config file path
    def get_fsc_config(self, fsc_config):
        if os.path.isfile(fsc_config):
            Logger.info("Started, reading configuration from %s" % (fsc_config))
            with open(fsc_config, "r") as f:
                return json.load(f)

    def get_config_params(self):
        self.transitional = self.fsc_config["pwm_transition_value"]
        self.boost = self.fsc_config["pwm_boost_value"]
        if "fan_limit_upper_pwm" in self.fsc_config:
            self.fan_limit_upper_pwm = self.fsc_config["fan_limit_upper_pwm"]
        if "fan_limit_lower_pwm" in self.fsc_config:
            self.fan_limit_lower_pwm = self.fsc_config["fan_limit_lower_pwm"]
        if "non_fanfail_limited_boost_value" in self.fsc_config:
            self.non_fanfail_limited_boost = self.fsc_config[
                "non_fanfail_limited_boost_value"
            ]
        self.sensor_filter_all = self.fsc_config.get("sensor_filter_all", False)
        self.sensor_fail_ignore = self.fsc_config.get("sensor_fail_ignore", False)
        if "boost" in self.fsc_config and "fan_fail" in self.fsc_config["boost"]:
            self.fan_fail = self.fsc_config["boost"]["fan_fail"]
        if "boost" in self.fsc_config and "progressive" in self.fsc_config["boost"]:
            if self.fsc_config["boost"]["progressive"]:
                self.boost_type = "progressive"
        if "fan_dead_boost" in self.fsc_config:
            self.fan_dead_boost = self.fsc_config["fan_dead_boost"]
            self.all_fan_fail_counter = 0
        if "output_max_boost_pwm" in self.fsc_config:
            self.output_max_boost_pwm = self.fsc_config["output_max_boost_pwm"]
        if "boost" in self.fsc_config and "sensor_fail" in self.fsc_config["boost"]:
            self.sensor_fail = self.fsc_config["boost"]["sensor_fail"]
            if self.sensor_fail:
                if "pwm_sensor_boost_value" in self.fsc_config:
                    self.pwm_sensor_boost_value = self.fsc_config[
                        "pwm_sensor_boost_value"
                    ]
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
            fscd_setup_watchdog()
        self.interval = self.fsc_config["sample_interval_ms"] / 1000.0
        if "fan_recovery_time" in self.fsc_config:
            self.fan_recovery_time = self.fsc_config["fan_recovery_time"]
        if "multi_fan_fail" in self.fsc_config:
            self.multi_fan_fail = self.fsc_config["multi_fan_fail"]
        if "standby_fan_fail" in self.fsc_config:
            self.standby_fan_fail = self.fsc_config["standby_fan_fail"]


    def build_profiles(self):
        self.sensors = {}
        self.profiles = {}

        for name, pdata in list(self.fsc_config["profiles"].items()):
            sensor = Sensor(name, pdata)
            if isinstance(sensor.source, FscSensorSourceJson):
                self.machine.extra_sensors[name] = sensor
            self.sensors[name] = sensor
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
                    self.profiles,
                    self.transitional,
                    counter,
                    self.boost,
                    self.sensor_fail,
                    self.sensor_valid_check,
                    self.fail_sensor_type,
                    self.ssd_progressive_algorithm,
                    self.sensor_fail_ignore,
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
                            # Use dict.get(key, None) avoid exception when no configuration
                            valid_fault_tolerant = valid_table.get(
                                "fault_tolerant", None
                            )
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
                                        if valid_fault_tolerant:
                                            Logger.info(
                                                "%s without action since fault_tolerant is enabled"
                                                % (reason)
                                            )
                                            continue
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
                if tuple.value is None:  # Skip sensor if the reading fail
                    continue
                if tuple.name in self.fsc_config["profiles"]:
                    last_error_time = self.sensors[tuple.name].source.last_error_time
                    last_error_level = self.sensors[tuple.name].source.last_error_level
                    if "read_limit" in self.fsc_config["profiles"][tuple.name]:
                        # If temperature read exceeds accpetable temperature reading
                        if (
                            "alarm_major"
                            in self.fsc_config["profiles"][tuple.name]["read_limit"]
                        ):
                            # Get value from configuration
                            valid_table = self.fsc_config["profiles"][tuple.name][
                                "read_limit"
                            ]["alarm_major"]
                            valid_read_limit = valid_table["limit"]
                            # Compare temp after offset with threshold value
                            if tuple.value >= valid_read_limit:
                                reason = (
                                    sensor
                                    + "(alarm_major v="
                                    + str(tuple.value)
                                    + ") limit(t="
                                    + str(valid_read_limit)
                                    + ") reached"
                                )
                                # change error flag and update last time error for Soak time feature
                                last_error_level = "alarm_major"
                                self.sensors[tuple.name].source.last_error_time = int(
                                    datetime.datetime.now().strftime("%s")
                                )
                                # Do action if exist in configuration
                                if "action" in valid_table:
                                    self.fsc_host_action(
                                        action=valid_table["action"], cause=reason
                                    )
                                Logger.warn(reason)
                                ret = 1
                                # Skip alarm minor
                                continue

                        if (
                            "alarm_minor"
                            in self.fsc_config["profiles"][tuple.name]["read_limit"]
                        ):
                            # Get value from configuration
                            valid_table = self.fsc_config["profiles"][tuple.name][
                                "read_limit"
                            ]["alarm_minor"]
                            valid_read_limit = valid_table["limit"]
                            # Compare temp after offset with threshold value
                            if tuple.value >= valid_read_limit:
                                reason = (
                                    sensor
                                    + "(alarm_minor v="
                                    + str(tuple.value)
                                    + ") limit(t="
                                    + str(valid_read_limit)
                                    + ") reached"
                                )
                                # change error flag and update last time error for Soak time feature
                                if last_error_level is None:
                                    last_error_level = "alarm_minor"
                                self.sensors[tuple.name].source.last_error_time = int(
                                    datetime.datetime.now().strftime("%s")
                                )
                                # Do action if exist in configuration
                                if "action" in valid_table:
                                    self.fsc_host_action(
                                        action=valid_table["action"], cause=reason
                                    )
                                Logger.warn(reason)
                                ret = 1
                            elif (
                                "soak_time_s" in valid_table
                                and last_error_level is not None
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
                                    if "hysteresis" in valid_table:
                                        valid_hysteresis = abs(
                                            valid_table["hysteresis"]
                                        )
                                        if tuple.value > (
                                            valid_read_limit - valid_hysteresis
                                        ):
                                            reason = (
                                                sensor
                                                + "(alarm_minor current v="
                                                + str(tuple.value)
                                                + ") target(t="
                                                + str(
                                                    valid_read_limit - valid_hysteresis
                                                )
                                                + ") soak_count (n="
                                                + str(
                                                    self.sensors[
                                                        tuple.name
                                                    ].source.soak_repeat_counter
                                                    + 1
                                                )
                                                + ") repeating"
                                            )
                                            self.sensors[
                                                tuple.name
                                            ].source.last_error_time = int(
                                                datetime.datetime.now().strftime("%s")
                                            )
                                            self.sensors[
                                                tuple.name
                                            ].source.soak_repeat_counter += 1
                                            Logger.warn(reason)
                                            ret = 1
                                        else:
                                            last_error_level = None
                                    else:
                                        last_error_level = None

                        self.sensors[
                            tuple.name
                        ].source.last_error_level = last_error_level
        return ret

    def check_fan_rearm(self):
        try:
            status = kv.kv_get(FAN_DEAD_REARM_KEY, 1)
        except kv.KeyNotFoundFailure:
            return False

        if status == "1":
            kv.kv_set(FAN_DEAD_REARM_KEY, "0", 1)
            return True
        else:
            return False

    def update_dead_fans(self, dead_fans):
        """
        Check for dead and recovered fans
        """
        last_dead_fans = dead_fans.copy()
        speeds = self.machine.read_fans(self.fans)

        for fan, rpms in list(speeds.items()):
            Logger.info("%s speed: %d RPM" % (fan.label, rpms))
            if rpms < self.fsc_config["min_rpm"]:
                dead_fans.add(fan)
                self.fsc_fan_action(fan, action="dead")
            else:
                dead_fans.discard(fan)

        recovered_fans = last_dead_fans - dead_fans
        newly_dead_fans = dead_fans - last_dead_fans

        if len(newly_dead_fans) > 0 or (self.need_rearm and len(dead_fans) > 0):
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
                    try:
                        fan_fail_record = open(fan_fail_record_path, "w")
                        fan_fail_record.close()
                    except FileNotFoundError:
                        Logger.warn(
                            "Cannot create failure record for %s" % (dead_fan.label)
                        )
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
        self.need_rearm = self.check_fan_rearm()
        ctx = {}
        if not self.sensor_filter_all:
            sensors_tuples = self.machine.read_sensors(self.sensors, None)
            self.fsc_safe_guards(sensors_tuples)
        for zone in self.zones:
            if self.need_rearm:
                zone.transitional_assert_flag = False
                zone.missing_sensor_assert_flag = [False] * len(
                    zone.expr_meta["ext_vars"]
                )
            if self.sensor_filter_all:
                sensors_tuples = self.machine.read_sensors(self.sensors, zone.expr_meta)
                self.fsc_safe_guards(sensors_tuples)
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
            Logger.debug(" dead_fans(%d) " % len(dead_fans))
            Logger.debug("Calculate")

            if chassis_intrusion_boost_flag == 0 and sensor_violated_flag == 0:
                ctx["dt"] = time_difference
                ctx["dead_fans"] = dead_fans
                ctx["last_pwm"] = zone.last_pwm
                ignore_fan_mode = False
                if self.non_fanfail_limited_boost and dead_fans:
                    ignore_fan_mode = True
                pwmval = zone.run(
                    sensors=sensors_tuples, ctx=ctx, ignore_mode=ignore_fan_mode
                )
                mode = zone.get_set_fan_mode(mode, action="read")
                # if we set pwm_sensor_boost_value option, assign it to pwmval
                if (
                    self.pwm_sensor_boost_value != None
                    and int(mode) == fan_mode["boost_mode"]
                ):
                    if pwmval == self.boost:
                        pwmval = self.pwm_sensor_boost_value
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
                        dead = len(dead_fans)
                        # If not progressive ,when there is 1 fan failed, boost all fans
                        Logger.info(
                            "Failed fans: %s"
                            % (", ".join([str(i.label) for i in dead_fans]))
                        )
                        if self.standby_fan_fail and not self.get_fan_power_status():
                            mode = fan_mode["standby_boost_mode"]
                            pwmval = self.standby_fan_fail["fan_pwm"]

                        elif dead > 1 and self.multi_fan_fail:
                            for fan_count, set_fan_pwm in self.multi_fan_fail["data"]:
                                if dead >= fan_count:
                                    #choose the higher PWM
                                    pwmval = max(set_fan_pwm, pwmval)
                                    if int(pwmval) == int(set_fan_pwm):
                                        mode = fan_mode["boost_mode"]
                                    else:
                                        pwmval = zone.run(
                                        sensors=sensors_tuples, ctx=ctx, ignore_mode=False
                                        )
                                        mode = zone.get_set_fan_mode(mode, action="read")

                            if "host_action" in list(self.multi_fan_fail.keys()):
                                act, cnt = self.multi_fan_fail["host_action"]
                                if act == "shutdown" and dead >= cnt:
                                    if (self.get_fan_power_status()):
                                        self.fsc_host_action(
                                            action="host_shutdown",
                                            cause="Bad fan count exceeded threshold: "
                                            + str(fan_count),
                                        )
                                        mode = fan_mode["standby_boost_mode"]

                        elif self.board_fan_mode.is_scenario_supported("one_fan_failure"):
                            # user define
                            (
                                set_fan_mode,
                                set_fan_pwm,
                            ) = self.board_fan_mode.get_board_fan_mode(
                                "one_fan_failure"
                            )
                            # choose the higher PWM
                            pwmval = max(set_fan_pwm, pwmval)
                            if int(pwmval) == int(set_fan_pwm):
                                mode = set_fan_mode
                            else:
                                pwmval = zone.run(
                                    sensors=sensors_tuples, ctx=ctx, ignore_mode=False
                                )
                                mode = zone.get_set_fan_mode(mode, action="read")
                        else:
                            # choose the higher PWM
                            if self.output_max_boost_pwm:
                                pwmval = self.boost if pwmval < self.boost else pwmval
                            else:
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

            if self.fan_limit_upper_pwm:
                if pwmval > self.fan_limit_upper_pwm:
                    pwmval = self.fan_limit_upper_pwm

            if self.fan_limit_lower_pwm:
                if pwmval < self.fan_limit_lower_pwm:
                    pwmval = self.fan_limit_lower_pwm

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
            Logger.info("time_difference: %f" % (time_difference))

            # Check sensors and update zones
            self.update_zones(dead_fans, time_difference)


def handle_term(signum, frame):
    board_callout(callout="init_fans", boost=DEFAULT_INIT_TRANSITIONAL)
    Logger.warn("killed by signal %d" % (signum,))
    if signum == signal.SIGQUIT:
        fscd_release_watchdog(stop_wdt=True)
    else:
        fscd_release_watchdog()
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
        fscd_release_watchdog()
