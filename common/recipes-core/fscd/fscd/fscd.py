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
from lib_pal import *

from fsc_control import PID, TTable
import fsc_expr

RAMFS_CONFIG = '/etc/fsc-config.json'
CONFIG_DIR = '/etc/fsc'

boost = 100
boost_type = 'default'
transitional = 70
ramp_rate = 10
verbose = "-v" in sys.argv

SensorValue = namedtuple('SensorValue', ['id','name','value','unit','status'])

def bmc_symbolize_sensorname(name):
    return name.lower().replace(" ", "_")

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
                symname = bmc_symbolize_sensorname(name)
                result[symname] = SensorValue(sid, name, value, None, status)
                continue
        m = re.match(r"^(.*)\((0x..?)\)\s+:\s+([^\s]+)\s+([^\s]+)\s+.\s+\((.+)\)$", line)
        if m is not None:
            sid = int(m.group(2), 16)
            name = m.group(1).strip()
            value = float(m.group(3))
            unit = m.group(4)
            status = m.group(5)
            symname = bmc_symbolize_sensorname(name)
            result[symname] = SensorValue(sid, name, value, unit, status)
    return result

def bmc_read_speed():
    cmd = '/usr/local/bin/fan-util --get'
    data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    sdata = data.split('\n')
    result = {}
    fan_n = 0
    for line in sdata:
        m = re.match(r"Fan .*\sSpeed:\s+(\d+)\s", line)
        if m is not None:
            result[fan_n] = int(m.group(1))
            fan_n += 1
    return result

# TODO: Revisit this when another platform needs to check power status
def bmc_read_power():
    cmd = '/usr/local/bin/power-util mb status'
    data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    if 'ON' in data:
        return 1
    else:
        return 0

class BMCMachine:
    def __init__(self):
        self.frus = set()
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
    def read_power(self):
        return bmc_read_power()
    def read_sensors(self):
        sensors = {}
        for fru in self.frus:
            sensors[fru] = bmc_sensor_read(fru)
        return sensors

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
        controller = TTable(
                profile['data'],
                profile.get('negative_hysteresis', 0),
                profile.get('positive_hysteresis', 0))
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

class Zone:
    def __init__(self, pwm_output, expr, expr_meta):
        self.pwm_output = pwm_output
        self.last_pwm = transitional
        self.expr = expr
        self.expr_meta = expr_meta
        self.expr_str = str(expr)

    def run(self, sensors, dt):
        ctx = {'dt': dt}
        outmin = 0
        missing = set()
        for v in self.expr_meta['ext_vars']:
            board, sname = v.split(":")
            if sname in sensors[board]:
                sensor = sensors[board][sname]
                ctx[v] = sensor.value
                if sensor.status in ['ucr', 'unr', 'lnr', 'lcr']:
                    warn('Sensor %s reporting status %s' %
                         (sensor.name, sensor.status))
                    outmin = transitional
            else:
                missing.add(v)
                # evaluation tries to ignore the effects of None values
                # (e.g. acts as 0 in max/+)
                ctx[v] = None
        if missing:
            warn('Missing sensors: %s' % (', '.join(missing),))
        if verbose:
            (exprout, dxstr) = self.expr.dbgeval(ctx)
            print(dxstr + " = " + str(exprout))
        else:
            exprout = self.expr.eval(ctx)
            print(self.expr_str + " = " + str(exprout))
        # If *all* sensors in the top level max() report None, the
        # expression will report None
        if not exprout:
            crit('No sane fan speed could be calculated! Using transitional speed.')
            exprout = transitional
        if exprout < outmin:
            exprout = outmin
        exprout = clamp(exprout, 0, 100)
        return exprout

def profile_constructor(data):
    return lambda: make_controller(data)

def main():
    global transitional
    global boost
    global boost_type
    global wdfile
    global ramp_rate
    syslog.openlog("fscd")
    info("starting")
    machine.set_all_pwm(transitional)
    configfile = "config.json"
    config = None
    zones = []
    if os.path.isfile(RAMFS_CONFIG):
        configfile = RAMFS_CONFIG
    info("Started, reading configuration from %s" % (configfile,))
    with open(configfile, 'r') as f:
        config = json.load(f)
    transitional = config['pwm_transition_value']
    boost = config['pwm_boost_value']
    if 'boost' in config and 'progressive' in config['boost']:
        if config['boost']['progressive']:
            boost_type = 'progressive'
    watchdog = config['watchdog']
    if 'fanpower' in config:
        fanpower = config['fanpower']
    else:
        fanpower = False
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
    profile_constructors = {}
    for name, pdata in config['profiles'].items():
        profile_constructors[name] = profile_constructor(pdata)

    print("Available profiles: " + ", ".join(profile_constructors.keys()))

    for name, data in config['zones'].items():
        filename = data['expr_file']
        with open(os.path.join(CONFIG_DIR, filename), 'r') as exf:
            source = exf.read()
            print("Compiling FSC expression for zone:")
            print(source)
            (expr, inf) = fsc_expr.make_eval_tree(source, profile_constructors)
            for name in inf['ext_vars']:
                board, sname = name.split(':')
                machine.frus.add(board)
            zone = Zone(data['pwm_output'], expr, inf)
            zones.append(zone)
    info("Read %d zones" % (len(zones),))
    info("Including sensors from: " + ", ".join(machine.frus))
    interval = config['sample_interval_ms'] / 1000.0

    last = time.time()
    dead_fans = set()
    pwr_status = False

    if fanpower:
        time.sleep(30)

    while True:
        last_dead_fans = dead_fans.copy()
        if wdfile:
            wdfile.write('V')
            wdfile.flush()

        time.sleep(interval)
        speeds = machine.read_speed()
        if fanpower:
            status = machine.read_power()
            if status == 0:
                pwr_status = False
                continue
            else:
                if pwr_status == False:
                    pwr_status = True
                    continue
        sensors = machine.read_sensors()
        fan_fail = False
        now = time.time()
        dt = now - last
        last = now
        print("\x1b[2J\x1b[H")
        sys.stdout.flush()
        for fan, rpms in speeds.items():
            print("Fan %d speed: %d RPM" % (fan, rpms))
            if rpms < config['min_rpm']:
                dead_fans.add(fan)
                pal_fan_dead_handle(fan)
            else:
                dead_fans.discard(fan)
        recovered_fans = last_dead_fans - dead_fans
        newly_dead_fans = dead_fans - last_dead_fans
        if len(newly_dead_fans) > 0:
            if fanpower:
                warn("%d fans failed" % (len(dead_fans),))
            else:
                crit("%d fans failed" % (len(dead_fans),))
            for dead_fan_num in dead_fans:
                if fanpower:
                    warn("Fan %d dead, %d RPM" % (dead_fan_num, rpms))
                else:
                    crit("Fan %d dead, %d RPM" % (dead_fan_num, rpms))
        for fan in recovered_fans:
            if fanpower:
                warn("Fan %d has recovered" % (fan,))
            else:
                crit("Fan %d has recovered" % (fan,))
            pal_fan_recovered_handle(fan)
        for zone in zones:
            print("PWM: %s" % (json.dumps(zone.pwm_output)))
            pwmval = zone.run(sensors, dt)
            if abs(zone.last_pwm - pwmval) > ramp_rate:
                if pwmval < zone.last_pwm:
                    pwmval = zone.last_pwm - ramp_rate
                else:
                    pwmval = zone.last_pwm + ramp_rate
            zone.last_pwm = pwmval
            if boost_type == 'progressive':
                dead = len(dead_fans)
                if dead > 0:
                    print("Failed fans: %s" %
                      (', '.join([str(i) for i in dead_fans],)))
                    if dead < 3:
                        pwmval = clamp(pwmval + (10 * dead), 0, 100)
                        print("Boosted PWM to %d" % pwmval)
                    else:
                        pwmval = boost
            else:
                if dead_fans:
                    print("Failed fans: %s" %
                          (', '.join([str(i) for i in dead_fans],)))
                    pwmval = boost
            if hasattr(zone.pwm_output, '__iter__'):
                for output in zone.pwm_output:
                    machine.set_pwm(output, pwmval)
            else:
                machine.set_pwm(zone.pwm_output, pwmval)

def handle_term(signum, frame):
    global wdfile
    machine.set_all_pwm(transitional)
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
        machine.set_all_pwm(transitional)
        (etype, e) = sys.exc_info()[:2]
        warn("failed, exception: " + str(etype))
        traceback.print_exc()
