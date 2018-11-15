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
import os.path
from fsc_util import Logger, clamp
from fsc_sensor import FscSensorSourceSysfs, FscSensorSourceUtil
import fsc_board


verbose = "-v" in sys.argv
RECORD_DIR = '/tmp/cache_store/'
SENSOR_FAIL_RECORD_DIR = '/tmp/sensorfail_record/'
fan_mode = {
    'normal_mode': 0,
    'trans_mode': 1,
    'boost_mode': 2
}

class Fan(object):
    def __init__(self, fan_name, pTable):
        try:
            self.fan_num = int(fan_name)
            if 'label' in pTable:
                self.label = pTable['label']
            else:
                self.label = "Fan %d" % (self.fan_num)

            if 'sysfs' in pTable['read_source']:
                if 'write_source' in pTable:
                    self.source = FscSensorSourceSysfs(
                        name=fan_name,
                        read_source=pTable['read_source']['sysfs'],
                        write_source=pTable['write_source']['sysfs'])
                else:
                    self.source = FscSensorSourceSysfs(
                        name=fan_name,
                        read_source=pTable['read_source']['sysfs'])
            if 'util' in pTable['read_source']:
                if 'write_source' in pTable:
                    self.source = FscSensorSourceUtil(
                        name=fan_name,
                        read_source=pTable['read_source']['util'],
                        write_source=pTable['write_source']['util'])
                else:
                    self.source = FscSensorSourceUtil(
                        name=fan_name,
                        read_source=pTable['read_source']['util'])
        except Exception:
            Logger.error("Unknown Fan source type")


class Zone:
    def __init__(self, pwm_output, expr, expr_meta, transitional, counter, boost, sensor_fail, sensor_valid_check, fail_sensor_type, ssd_progressive_algorithm):
        self.pwm_output = pwm_output
        self.last_pwm = transitional
        self.transitional = transitional
        self.expr = expr
        self.expr_meta = expr_meta
        self.expr_str = str(expr)
        self.transitional_assert_flag = False
        self.counter = counter
        self.boost = boost
        self.sensor_fail = sensor_fail
        self.sensor_valid_check = sensor_valid_check
        self.fail_sensor_type = fail_sensor_type
        self.ssd_progressive_algorithm = ssd_progressive_algorithm
        self.missing_sensor_assert_flag = ([False] * len(self.expr_meta['ext_vars']))
        self.missing_sensor_assert_retry = ([0] * len(self.expr_meta['ext_vars']))
        self.sensor_valid_pre = ([0] * len(self.expr_meta['ext_vars']))
        self.sensor_valid_cur = ([0] * len(self.expr_meta['ext_vars']))

    def get_set_fan_mode(self, mode, action):
        fan_mode_path = RECORD_DIR + 'fan_mode'
        if not os.path.isdir(RECORD_DIR):
            os.mkdir(RECORD_DIR)
        if action in 'read':
            if os.path.isfile(fan_mode_path):
                with open(fan_mode_path, "r") as f:
                    mode = f.read(1)
                return mode
            else:
                return fan_mode['normal_mode']
        elif action in 'write':
            if os.path.isfile(fan_mode_path):
                with open(fan_mode_path, "r") as f:
                    mode_tmp = f.read(1)
                if mode != mode_tmp:
                    fan_mode_record = open(fan_mode_path, 'w')
                    fan_mode_record.write(str(mode))
                    fan_mode_record.close()
            else:
                fan_mode_record = open(fan_mode_path, 'w')
                fan_mode_record.write(str(mode))
                fan_mode_record.close()

    def run(self, sensors, dt):
        ctx = {'dt': dt}
        outmin = 0
        fail_ssd_count = 0
        sensor_index = 0
        cause_boost_count = 0
        no_sane_flag = 0
        mode = 0

        for v in self.expr_meta['ext_vars']:
            sensor_valid_flag = 1
            board, sname = v.split(":")
            if self.sensor_valid_check != None:
                for check_name in self.sensor_valid_check:
                    if re.match(check_name, sname, re.IGNORECASE) != None:
                        self.sensor_valid_cur[sensor_index] = fsc_board.sensor_valid_check(board, sname, check_name, self.sensor_valid_check[check_name]["attribute"])
                        #If current or previous sensor valid status is 0, ignore this sensor reading.
                        #Only when both are 1, goes to sensor check process
                        if (self.sensor_valid_cur[sensor_index] == 0) or (self.sensor_valid_pre[sensor_index] == 0):
                            sensor_valid_flag = 0
                            self.missing_sensor_assert_retry[sensor_index] = 0
                        break

            if sensor_valid_flag == 1:
                if sname in sensors[board]:
                    self.missing_sensor_assert_retry[sensor_index] = 0
                    if self.missing_sensor_assert_flag[sensor_index]:
                        Logger.crit('DEASSERT: Zone%d Missing sensors: %s' % (self.counter, v))
                        self.missing_sensor_assert_flag[sensor_index] = False

                    sensor = sensors[board][sname]
                    ctx[v] = sensor.value
                    if sensor.status in ['ucr']:
                        Logger.warn('Sensor %s reporting status %s' % (sensor.name, sensor.status))
                        outmin = max(outmin, self.transitional)
                        if outmin == self.transitional:
                            mode = fan_mode['trans_mode']
                    else:
                        if self.sensor_fail == True:
                            sensor_fail_record_path = SENSOR_FAIL_RECORD_DIR + v
                            if not os.path.isdir(SENSOR_FAIL_RECORD_DIR):
                                os.mkdir(SENSOR_FAIL_RECORD_DIR)
                            if (sensor.status in ['na']) and (self.sensor_valid_cur[sensor_index] != -1):
                                if re.match(r'SSD', sensor.name) != None:
                                    fail_ssd_count = fail_ssd_count + 1
                                else:
                                    Logger.warn("%s Fail" % v)
                                    outmin = max(outmin, self.boost)
                                    cause_boost_count += 1
                                if not os.path.isfile(sensor_fail_record_path):
                                    sensor_fail_record = open(sensor_fail_record_path, 'w')
                                    sensor_fail_record.close()
                                if outmin == self.boost:
                                    mode = fan_mode['boost_mode']
                            else:
                                if os.path.isfile(sensor_fail_record_path):
                                    os.remove(sensor_fail_record_path)
                else:
                    if (not self.missing_sensor_assert_flag[sensor_index]) and (self.missing_sensor_assert_retry[sensor_index] >= 2):
                        Logger.crit('ASSERT: Zone%d Missing sensors: %s' % (self.counter, v))
                        self.missing_sensor_assert_flag[sensor_index] = True
                    if (self.missing_sensor_assert_retry[sensor_index] < 2):
                        self.missing_sensor_assert_retry[sensor_index] += 1
                    # evaluation tries to ignore the effects of None values
                    # (e.g. acts as 0 in max/+)
                    ctx[v] = None
            self.sensor_valid_pre[sensor_index] = self.sensor_valid_cur[sensor_index]
            sensor_index += 1

        if verbose:
            (exprout, dxstr) = self.expr.dbgeval(ctx)
            Logger.info(dxstr + " = " + str(exprout))
        else:
            exprout = self.expr.eval(ctx)
            Logger.info(self.expr_str + " = " + str(exprout))
        # If *all* sensors in the top level max() report None, the
        # expression will report None
        if (not exprout) and (outmin == 0):
            if not self.transitional_assert_flag:
                Logger.crit('ASSERT: Zone%d No sane fan speed could be \
                    calculated! Using transitional speed.' % (self.counter))
            exprout = self.transitional
            mode = fan_mode['trans_mode']
            no_sane_flag = 1
            self.transitional_assert_flag = True
        else:
            if self.transitional_assert_flag:
                Logger.crit('DEASSERT: Zone%d No sane fan speed could be \
                    calculated! Using transitional speed.' % (self.counter))
            self.transitional_assert_flag = False

        if self.fail_sensor_type != None:
            if 'SSD_sensor_fail' in list(self.fail_sensor_type.keys()):
                if self.fail_sensor_type['SSD_sensor_fail'] == True:
                    if fail_ssd_count != 0:
                        if self.ssd_progressive_algorithm != None:
                            if 'offset_algorithm' in list(self.ssd_progressive_algorithm.keys()):
                                list_index = 0
                                for i in self.ssd_progressive_algorithm['offset_algorithm']:
                                    list_index = list_index + 1
                                    if fail_ssd_count <= i[0]:
                                        exprout = exprout + i[1]
                                        no_sane_flag = 0
                                        break
                                    else:
                                        if list_index == len(self.ssd_progressive_algorithm['offset_algorithm']):
                                           outmin = max(outmin, self.boost)
                                           cause_boost_count += 1
                                           if outmin == self.boost:
                                               mode = fan_mode['boost_mode']

        boost_record_path = RECORD_DIR + 'sensor_fail_boost'
        if cause_boost_count != 0:
            if not os.path.isfile(boost_record_path):
                sensor_fail_boost_record = open(boost_record_path, 'w')
                sensor_fail_boost_record.close()
        else:
            if os.path.isfile(boost_record_path):
                os.remove(boost_record_path)

        if not exprout:
            exprout = 0
        if exprout < outmin:
            exprout = outmin
        else:
          if no_sane_flag != 1:
              mode = fan_mode['normal_mode']
        self.get_set_fan_mode(mode, action='write')
        exprout = clamp(exprout, 0, 100)
        return exprout
