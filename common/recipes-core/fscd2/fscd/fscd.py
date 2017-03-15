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
import json
import os.path
import fsc_expr
import time
import sys
import signal
import traceback

from fsc_util import Logger, clamp
from fsc_profile import profile_constructor, Sensor
from fsc_zone import Zone, Fan
from fsc_bmcmachine import BMCMachine
from fsc_board import board_fan_actions, board_host_actions, board_callout

RAMFS_CONFIG = '/etc/fsc-config.json'
CONFIG_DIR = '/etc/fsc'
DEFAULT_INIT_BOOST = 100


class Fscd(object):

    DEFAULT_BOOST = 100
    DEFAULT_BOOST_TYPE = 'default'
    DEFAULT_TRANSITIONAL = 70
    DEFAULT_RAMP_RATE = 10

    def __init__(self, config=RAMFS_CONFIG, zone_config=CONFIG_DIR):
        Logger.start("fscd")
        Logger.info("Starting fscd")
        self.zone_config = zone_config
        self.fsc_config = self.get_fsc_config(config)  # json dump from config
        self.boost = self.DEFAULT_BOOST
        self.boost_type = self.DEFAULT_BOOST_TYPE
        self.transitional = self.DEFAULT_TRANSITIONAL
        self.ramp_rate = self.DEFAULT_RAMP_RATE

    # TODO: Add checks for invalid config file path
    def get_fsc_config(self, fsc_config):
        if os.path.isfile(fsc_config):
            Logger.info("Started, reading configuration from %s" %
                        (fsc_config))
            with open(fsc_config, 'r') as f:
                return json.load(f)

    def get_config_params(self):
        self.transitional = self.fsc_config['pwm_transition_value']
        self.boost = self.fsc_config['pwm_boost_value']
        if 'boost' in self.fsc_config and 'progressive' in self.fsc_config['boost']:
                if self.fsc_config['boost']['progressive']:
                    self.boost_type = 'progressive'
        self.watchdog = self.fsc_config['watchdog']
        if 'fanpower' in self.fsc_config:
            self.fanpower = self.fsc_config['fanpower']
        else:
            self.fanpower = False
        if 'chassis_intrusion' in self.fsc_config:
            self.chassis_intrusion = True
        else:
            self.chassis_intrusion = False
        if 'ramp_rate' in self.fsc_config:
            self.ramp_rate = self.fsc_config['ramp_rate']
        self.wdfile = None
        if self.watchdog:
            Logger.info("watchdog pinging enabled")
            self.wdfile = open('/dev/watchdog', 'w+')
            if not self.wdfile:
                Logger.error("couldn't open watchdog device")
            else:
                self.wdfile.write('V')
                self.wdfile.flush()
        self.interval = self.fsc_config['sample_interval_ms'] / 1000.0

    def build_profiles(self):
        self.sensors = {}
        self.profiles = {}

        for name, pdata in self.fsc_config['profiles'].items():
            self.sensors[name] = Sensor(name, pdata)
            self.profiles[name] = profile_constructor(pdata)

    def build_fans(self):
        self.fans = {}
        for name, pdata in self.fsc_config['fans'].items():
            self.fans[name] = Fan(name, pdata)

    def build_zones(self):
        self.zones = []
        counter = 0
        for name, data in self.fsc_config['zones'].items():
            filename = data['expr_file']
            with open(os.path.join(self.zone_config, filename), 'r') as exf:
                source = exf.read()
                print("Compiling FSC expression for zone:")
                print(source)
                (expr, inf) = fsc_expr.make_eval_tree(source,
                                                      self.profiles)
                for name in inf['ext_vars']:
                    board, sname = name.split(':')
                    self.machine.frus.add(board)
                zone = Zone(data['pwm_output'], expr, inf, self.transitional,
                            counter)
                counter += 1
                self.zones.append(zone)

    def build_machine(self):
        self.machine = BMCMachine()

    def fsc_fan_action(self, fan, action):
        '''
        Method invokes board actions for a fan.
        '''
        if 'dead' in action:
            board_fan_actions(fan, action='dead')
        if 'recover' in action:
            board_fan_actions(fan, action='recover')

    def update_dead_fans(self, dead_fans):
        '''
        Check for dead and recovered fans
        '''
        last_dead_fans = dead_fans.copy()
        speeds = self.machine.read_fans(self.fans)
        print("\x1b[2J\x1b[H")
        sys.stdout.flush()

        for fan, rpms in speeds.items():
            print("Fan %d speed: %d RPM" % (fan, rpms))
            if rpms < self.fsc_config['min_rpm']:
                dead_fans.add(fan)
                self.fsc_fan_action(fan, action='dead')
            else:
                dead_fans.discard(fan)

        recovered_fans = last_dead_fans - dead_fans
        newly_dead_fans = dead_fans - last_dead_fans
        if len(newly_dead_fans) > 0:
            if self.fanpower:
                Logger.warn("%d fans failed" % (len(dead_fans),))
            else:
                Logger.crit("%d fans failed" % (len(dead_fans),))
            for dead_fan_num in dead_fans:
                if self.fanpower:
                    Logger.warn("Fan %d dead, %d RPM" % (dead_fan_num,
                                speeds[dead_fan_num]))
                else:
                    Logger.crit("Fan %d dead, %d RPM" % (dead_fan_num,
                                speeds[dead_fan_num]))
        for fan in recovered_fans:
            if self.fanpower:
                Logger.warn("Fan %d has recovered" % (fan,))
            else:
                Logger.crit("Fan %d has recovered" % (fan,))
            self.fsc_fan_action(fan, action='recover')
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

        """
        sensors_tuples = self.machine.read_sensors(self.sensors)
        for zone in self.zones:
            print("PWM: %s" % (json.dumps(zone.pwm_output)))

            chassis_intrusion_boost_flag = 0
            if self.chassis_intrusion:
                self_tray_pull_out = board_callout(
                    callout='chassis_intrusion')
                if self_tray_pull_out == 1:
                    chassis_intrusion_boost_flag = 1

            if chassis_intrusion_boost_flag == 0:
                pwmval = zone.run(sensors=sensors_tuples, dt=time_difference)
            else:
                pwmval = self.boost

            if self.boost_type == 'progressive':
                dead = len(dead_fans)
                if dead > 0:
                    print("Failed fans: %s" %
                          (', '.join([str(i) for i in dead_fans],)))
                    if dead < 3:
                        pwmval = clamp(pwmval + (10 * dead), 0, 100)
                        print("Boosted PWM to %d" % pwmval)
                    else:
                        pwmval = self.boost
            else:
                if dead_fans:
                    print("Failed fans: %s" % (
                        ', '.join([str(i) for i in dead_fans],)))
                    pwmval = self.boost

            if abs(zone.last_pwm - pwmval) > self.ramp_rate:
                if pwmval < zone.last_pwm:
                    pwmval = zone.last_pwm - self.ramp_rate
                else:
                    pwmval = zone.last_pwm + self.ramp_rate
            zone.last_pwm = pwmval

            if hasattr(zone.pwm_output, '__iter__'):
                for output in zone.pwm_output:
                    self.machine.set_pwm(self.fans.get(
                        unicode(str(output), 'utf-8')), pwmval)
            else:
                self.machine.set_pwm(self.fans[zone.pwm_output], pwmval)

    def builder(self):
        '''
        Method to extract from json and build all internal data staructures
        '''
        # Build a bmc machine object - read/write sensors
        self.build_machine()
        # Extract everything from json
        self.get_config_params()
        self.build_fans()
        self.build_profiles()
        Logger.info("Available profiles: " + ", ".join(self.profiles.keys()))
        self.build_zones()
        Logger.info("Read %d zones" % (len(self.zones)))
        Logger.info("Including sensors from: " + ", ".join(self.machine.frus))

    def get_fan_power_status(self):
        '''
        Method invokes board action to determine fan power status.
        If not applicable returns True.
        '''
        if self.fanpower:
            if board_callout(callout='read_power'):
                return True
        else:
                return True
        return False

    def run(self):
        """
        Main FSCD method that builds from the fscd config and runs
        """

        # Get everything from json and build profiles, fans, zones
        self.builder()

        self.machine.set_all_pwm(self.fans, self.transitional)

        last = time.time()
        dead_fans = set()

        if self.fanpower:
            time.sleep(30)

        while True:
            if self.wdfile:
                self.wdfile.write('V')
                self.wdfile.flush()

            time.sleep(self.interval)

            if not self.get_fan_power_status():
                continue

            # Get dead fans for determining speed
            dead_fans = self.update_dead_fans(dead_fans)

            now = time.time()
            time_difference = now - last
            last = now

            # Check sensors and update zones
            self.update_zones(dead_fans, time_difference)


def handle_term(signum, frame):
    global wdfile
    board_callout(callout='init_fans', boost=DEFAULT_INIT_BOOST)
    Logger.warn("killed by signal %d" % (signum,))
    if signum == signal.SIGQUIT and wdfile:
        Logger.info("Killed with SIGQUIT - stopping watchdog.")
        wdfile.write("X")
        wdfile.flush()
        wdfile.close()
        wdfile = None
    sys.exit('killed')


if __name__ == "__main__":
    try:
        signal.signal(signal.SIGTERM, handle_term)
        signal.signal(signal.SIGINT, handle_term)
        signal.signal(signal.SIGQUIT, handle_term)
        fscd = Fscd()
        fscd.run()
    except Exception:
        board_callout(callout='init_fans', boost=DEFAULT_INIT_BOOST)
        (etype, e) = sys.exc_info()[:2]
        Logger.crit("failed, exception: " + str(etype))
        traceback.print_exc()
