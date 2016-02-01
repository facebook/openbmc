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

import sys
import syslog
import json
import os.path
import traceback
from collections import namedtuple
import time
from subprocess import Popen, PIPE
import re
import signal

from fsc_control import PID, TTable


PERSISTENT_CONFIG = '/mnt/data/etc/fsc-config.json'
RAMFS_CONFIG = '/etc/fsc-config.json'

boost = 100
transitional = 70
ramp_rate = 10

SensorValue = namedtuple('SensorValue', ['id','name','value','unit','status'])

class FakeMachine:
    def set_pwm(self, pwm, pct):
        print("Set pwm %d to %d" % (pwm, pct))
    def set_all_pwm(self, pct):
        print("Set all pwm to %d" % (pct))
    def read_speed(self):
        return {0: 4000, 1: 4000}
    def read_sensors(self):
        return {'spb': {129: SensorValue(id=129, name='SP_INLET_TEMP', value=21.0, unit='C', status='ok')},
                'slot1': {9: SensorValue(id=9, name='SOC Therm Margin', value=-60.0, unit='C', status='ok')},
                'slot2': {9: SensorValue(id=9, name='SOC Therm Margin', value=-60.0, unit='C', status='ok')},
                'slot3': {9: SensorValue(id=9, name='SOC Therm Margin', value=-60.0, unit='C', status='ok')},
                'slot4': {9: SensorValue(id=9, name='SOC Therm Margin', value=-60.0, unit='C', status='ok')}}


# BAD. Lifted from REST API and patched up a bit
# TODO: add a --json to sensor-util

def bmc_sensor_read(fru):
    result = {}
    cmd = '/usr/local/bin/sensor-util ' + fru
    data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    sdata = data.split('\n')
    for line in sdata:
        # skip lines with " or startin with FRU
        if line.find("bic_read_sensor_wrapper") != -1:
            continue
        if line.find("failed") != -1:
            continue

        if line.find(" NA "):
            m = re.match(r"^(.*)\((0x..?)\)\s+:\s+([^\s]+)\s+.\s+\((.+)\)$", line)
            if m is not None:
                sid = int(m.group(2), 16)
                name = m.group(1).strip()
                value = None
                status = m.group(4)
                result[sid] = SensorValue(sid, name, value, None, status)
                continue
        m = re.match(r"^(.*)\((0x..?)\)\s+:\s+([^\s]+)\s+([^\s]+)\s+.\s+\((.+)\)$", line)
        if m is not None:
            sid = int(m.group(2), 16)
            name = m.group(1).strip()
            value = float(m.group(3))
            unit = m.group(4)
            status = m.group(5)
            result[sid] = SensorValue(sid, name, value, unit, status)
    return result

def bmc_read_speed():
    cmd = '/usr/local/bin/fan-util --get'
    data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    sdata = data.split('\n')
    result = {}
    for line in sdata:
        m = re.match(r"Fan (\d+)\sSpeed:\s+(\d+)\s", line)
        if m is not None:
            result[int(m.group(1))] = int(m.group(2))
    return result

class BMCMachine:
    def set_pwm(self, pwm, pct):
        print("Set pwm %d to %d" % (pwm, pct))
        cmd = ('/usr/local/bin/fan-util --set %d %d' % (pct, pwm))
        response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        if response.find("Setting") == -1:
            raise Exception(response)
    def set_all_pwm(self, pct):
        print("Set all pwm to %d" % (pct))
        cmd = ('/usr/local/bin/fan-util --set %d' % (pct))
        response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        if response.find("Setting") == -1:
            raise Exception(response)
    def read_speed(self):
        return bmc_read_speed()
    def read_sensors(self):
        sensors = {}
        for fru in ['spb', 'slot1', 'slot2', 'slot3', 'slot4']:
            sensors[fru] = bmc_sensor_read(fru)
        return sensors

#machine = FakeMachine()
machine = BMCMachine()
def info(msg):
    print("INFO: " + msg)
    syslog.syslog(syslog.LOG_INFO, msg)

def debug(msg):
    print("DEBUG: " + msg)

def warn(msg):
    print("WARNING: " + msg)
    syslog.syslog(syslog.LOG_WARNING, msg)

def error(msg):
    print("ERROR: " + msg)
    syslog.syslog(syslog.LOG_ERR, msg)

def crit(msg):
    print("CRITICAL: " + msg)
    syslog.syslog(syslog.LOG_CRIT, msg)

def make_controller(profile):
    if profile['type'] == 'linear':
        controller = TTable(profile['data'])
        return controller
    if profile['type'] == 'pid':
        controller = PID(
                profile['setpoint'],
                profile['kp'],
                profile['ki'],
                profile['kd'],
                profile['negative_hysteresis'],
                profile['positive_hysteresis'])
        return controller
    err = "Don't understand profile type '%s'" % (profile['type'])
    error(err)


def clamp(v, minv, maxv):
    if v <= minv:
        return minv
    if v >= maxv:
        return maxv
    return v

Input = namedtuple('Input', ['controller', 'sensor', 'profile'])

class Zone:
    def __init__(self, pwm_output):
        self.pwm_output = pwm_output
        self.inputs = []
        self.last_pwm = transitional

    def add_input(self, config, data):
        controller = make_controller(config['profiles'][data['profile']])
        self.inputs.append(Input(controller, data['sensor'], data['profile']))

    def run(self, sensors, dt):
        outs = []
        for i in self.inputs:
            if i.sensor['id'] not in sensors[i.sensor['board']]:
                warn('Unable to read sensor %s on board %s'
                        % (i.sensor['id'], i.sensor['board']))
                return boost
            sensor = sensors[i.sensor['board']][i.sensor['id']]
            if sensor.status == 'na':
                print("%s: NA, %s: -" % (sensor.name, i.profile))
                continue
            out = i.controller.run(sensor.value, dt)
            if out:
                out = clamp(out, 0, 100)
                outs.append(out)
            else:
                out = "-"
            print("%s: %.02f %s, %s: %s" %
                  (sensor.name, sensor.value, sensor.unit, i.profile, str(out)))
            if sensor.status in ['ucr', 'unr', 'lnr', 'lcr']:
                warn('Sensor %s reporting status %s' % (sensor.name, sensor.status))
                return boost
        if len(outs) == 0:
            # no sensors could be read
            return boost
        return max(outs)

def main():
    global transitional
    global boost
    global wdfile
    global ramp_rate
    syslog.openlog("fscd")
    info("starting")
    machine.set_all_pwm(transitional)
    configfile = "config.json"
    config = None
    zones = []
    if os.path.isfile(PERSISTENT_CONFIG):
        configfile = PERSISTENT_CONFIG
    elif os.path.isfile(RAMFS_CONFIG):
        configfile = RAMFS_CONFIG
    info("Started, reading configuration from %s" % (configfile,))
    with open(configfile, 'r') as f:
        config = json.load(f)
    transitional = config['pwm_transition_value']
    boost = config['pwm_boost_value']
    watchdog = config['watchdog']
    if 'ramp_rate' in config:
        ramp_rate = config['ramp_rate']
    wdfile = None
    if watchdog:
        info("watchdog pinging enabled")
        wdfile = open('/dev/watchdog', 'w+')
        if not wdfile:
            error("couldn't open watchdog device")
        else:
            wdfile.write('V')
            wdfile.flush()

    machine.set_all_pwm(transitional)
    for name, data in config['zones'].items():
        zone = Zone(data['pwm_output'])
        for idata in data['inputs']:
            zone.add_input(config, idata)
        zones.append(zone)
    info("Read %d zones" % (len(zones),))
    interval = config['sample_interval_ms'] / 1000.0
    last = time.time()
    while True:
        if wdfile:
            wdfile.write('V')
            wdfile.flush()

        time.sleep(interval)
        sensors = machine.read_sensors()
        speeds = machine.read_speed()
        fan_fail = False
        now = time.time()
        dt = now - last
        last = now
        print("\x1b[2J\x1b[H")
        sys.stdout.flush()
        for fan, rpms in speeds.items():
            print("Fan %d speed: %d RPM" % (fan, rpms))
            if rpms < config['min_rpm']:
                warn("Fan %d below min speed, PWM at boost value" % (fan,))
                fan_fail = True
        for zone in zones:
            print("PWM: %s" % (json.dumps(zone.pwm_output)))
            pwmval = zone.run(sensors, dt)
            if abs(zone.last_pwm - pwmval) > ramp_rate:
                if pwmval < zone.last_pwm:
                    pwmval = zone.last_pwm - ramp_rate
                else:
                    pwmval = zone.last_pwm + ramp_rate
            zone.last_pwm = pwmval
            if fan_fail:
                pwmval = boost
            if hasattr(zone.pwm_output, '__iter__'):
                for output in zone.pwm_output:
                    machine.set_pwm(output, pwmval)
            else:
                machine.set_pwm(zone.pwm_output, pwmval)

def handle_term(signum, frame):
    global wdfile
    machine.set_all_pwm(boost)
    warn("killed by signal %d" % (signum,))
    if signum == signal.SIGQUIT and wdfile:
        info("Killed with SIGQUIT - stopping watchdog.")
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
        main()
    except:
        machine.set_all_pwm(boost)
        (etype, e) = sys.exc_info()[:2]
        crit("failed, exception: " + str(etype))
        traceback.print_exc()
