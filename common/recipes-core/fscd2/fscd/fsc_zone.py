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
from fsc_util import Logger, clamp
from fsc_sensor import FscSensorSourceSysfs, FscSensorSourceUtil


verbose = "-v" in sys.argv


class Fan(object):
    def __init__(self, fan_name, pTable):
        try:
            if 'sysfs' in pTable['read_source']:
                self.source = FscSensorSourceSysfs(
                    name=fan_name,
                    read_source=pTable['read_source']['sysfs'],
                    write_source=pTable['write_source']['sysfs'])

            if 'util' in pTable['read_source']:
                self.source = FscSensorSourceUtil(
                    name=fan_name,
                    read_source=pTable['read_source']['util'],
                    write_source=pTable['write_source']['util'])
        except Exception:
            Logger.error("Unknown Fan source type")


class Zone:
    def __init__(self, pwm_output, expr, expr_meta, transitional, counter):
        self.pwm_output = pwm_output
        self.last_pwm = transitional
        self.transitional = transitional
        self.expr = expr
        self.expr_meta = expr_meta
        self.expr_str = str(expr)
        self.transitional_assert_flag = False
        self.counter = counter

    def run(self, sensors, dt):
        ctx = {'dt': dt}
        outmin = 0
        missing = set()
        for v in self.expr_meta['ext_vars']:
            board, sname = v.split(":")
            if sname in sensors[board]:
                sensor = sensors[board][sname]
                ctx[v] = sensor.value
                if sensor.status in ['ucr']:
                    Logger.warn('Sensor %s reporting status %s' %
                                (sensor.name, sensor.status))
                    outmin = self.transitional
            else:
                missing.add(v)
                # evaluation tries to ignore the effects of None values
                # (e.g. acts as 0 in max/+)
                ctx[v] = None
        if missing:
            Logger.warn('Missing sensors: %s' % (', '.join(missing),))
        if verbose:
            (exprout, dxstr) = self.expr.dbgeval(ctx)
            print(dxstr + " = " + str(exprout))
        else:
            exprout = self.expr.eval(ctx)
            print(self.expr_str + " = " + str(exprout))
        # If *all* sensors in the top level max() report None, the
        # expression will report None
        if not exprout:
            if not self.transitional_assert_flag:
                Logger.crit('ASSERT: Zone%d No sane fan speed could be \
                    calculated! Using transitional speed.' % (self.counter))
            exprout = self.transitional
            self.transitional_assert_flag = True
        else:
            if self.transitional_assert_flag:
                Logger.crit('DEASSERT: Zone%d No sane fan speed could be \
                    calculated! Using transitional speed.' % (self.counter))
            self.transitional_assert_flag = False
        if exprout < outmin:
            exprout = outmin
        exprout = clamp(exprout, 0, 100)
        return exprout
