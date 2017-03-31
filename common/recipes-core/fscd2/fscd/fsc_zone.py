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
import re
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
    def __init__(self, pwm_output, expr, expr_meta, transitional, counter, boost, fail_sensor_type, ssd_progressive_algorithm):
        self.pwm_output = pwm_output
        self.last_pwm = transitional
        self.transitional = transitional
        self.expr = expr
        self.expr_meta = expr_meta
        self.expr_str = str(expr)
        self.transitional_assert_flag = False
        self.counter = counter
        self.boost = boost
        self.fail_sensor_type = fail_sensor_type
        self.ssd_progressive_algorithm = ssd_progressive_algorithm

    def run(self, sensors, dt):
        ctx = {'dt': dt}
        outmin = 0
        fail_ssd_count = 0
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

                if self.fail_sensor_type != None:
                    if 'standby_sensor_fail' in self.fail_sensor_type.keys():
                        if self.fail_sensor_type['standby_sensor_fail'] == True:
                            if sensor.status in ['na']:
                                if re.match(r'SSD', sensor.name) == None:
                                    outmin = self.boost
                                    break
                                else:
                                    if 'SSD_sensor_fail' in self.fail_sensor_type.keys():
                                        if self.fail_sensor_type['SSD_sensor_fail'] ==True:
                                            fail_ssd_count = fail_ssd_count + 1
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

        if self.fail_sensor_type != None:
            if 'SSD_sensor_fail' in self.fail_sensor_type.keys():
                if self.fail_sensor_type['SSD_sensor_fail'] == True:
                    if fail_ssd_count != 0:
                        if self.ssd_progressive_algorithm != None:
                            if 'offset_algorithm' in self.ssd_progressive_algorithm.keys():
                                list_index = 0
                                for i in self.ssd_progressive_algorithm['offset_algorithm']:
                                    list_index = list_index + 1
                                    if fail_ssd_count <= i[0]:
                                        exprout = exprout + i[1]
                                        break
                                    else:
                                        if list_index == len(self.ssd_progressive_algorithm['offset_algorithm']):
                                            outmin = self.boost                    

        if exprout < outmin:
            exprout = outmin
        exprout = clamp(exprout, 0, 100)
        return exprout
