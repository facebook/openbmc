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
import math

from fsc_util import Logger


class PID:
    def __init__(self, setpoint, kp=0.0, ki=0.0, kd=0.0, neg_hyst=0.0, pos_hyst=0.0):
        self.last_error = 0
        self.I = 0
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.minval = setpoint - neg_hyst
        self.maxval = setpoint + pos_hyst
        self.last_out = None

    def run(self, value, ctx):
        dt = ctx["dt"]
        # don't accumulate into I term below min hysteresis
        if value < self.minval:
            self.I = 0
            self.last_out = None
        # calculate PID values above max hysteresis
        if value > self.maxval:
            error = self.maxval - value
            self.I = self.I + error * dt
            D = (error - self.last_error) / dt
            out = self.kp * error + self.ki * self.I + self.kd * D
            self.last_out = out
            self.last_error = error
            return out
        # use most recently calc'd PWM value
        return self.last_out


class IncrementPID:
    def __init__(self, setpoint, kp=0.0, ki=0.0, kd=0.0, neg_hyst=0.0, pos_hyst=0.0):
        self.setpoint = setpoint
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.valp1 = 0
        self.valp2 = 0
        self.minval = setpoint - neg_hyst
        self.maxval = setpoint + pos_hyst
        self.last_out = 0

    def run(self, value, ctx):
        value = float(value)
        self.last_out = ctx["last_pwm"]
        out = (
            (self.last_out)
            + (self.kp * (value - self.valp1))
            + (self.ki * (value - self.setpoint))
            + (self.kd * (value - 2 * self.valp1 + self.valp2))
        )
        self.valp2 = self.valp1
        self.valp1 = value
        Logger.debug(
            " PID  pwm(new:%.2f old:%.2f) pid(%.2f,%.2f,%.2f) setpoint(%.2f) temp(%.2f) "
            % (out, self.last_out, self.kp, self.ki, self.kd, self.setpoint, value)
        )

        self.last_out = out
        return self.last_out


# Threshold table
class TTable:
    def __init__(self, table, neg_hyst=0.0, pos_hyst=0.0):
        self.table = sorted(table, key=lambda in_thr_out: in_thr_out[0], reverse=True)
        self.compare_fsc_value = 0
        self.last_out = None
        self.neghyst = neg_hyst
        self.poshyst = pos_hyst

    def run(self, value, ctx):
        mini = 0

        if value >= self.compare_fsc_value:
            if math.fabs(self.compare_fsc_value - value) <= self.poshyst:
                return self.last_out

        if value <= self.compare_fsc_value:
            if math.fabs(self.compare_fsc_value - value) <= self.neghyst:
                return self.last_out

        for (in_thr, out) in self.table:
            mini = out
            if value >= in_thr:
                self.compare_fsc_value = value
                self.last_out = out
                return out

        self.compare_fsc_value = value
        self.last_out = mini
        return mini


class TTable4Curve:
    # Threshold table with 4 curve
    def __init__(
        self, table_normal_up, table_normal_down, table_onefail_up, table_onefail_down
    ):
        self.table_normal_up = sorted(
            table_normal_up, key=lambda in_thr_out: in_thr_out[0], reverse=True
        )
        self.table_normal_down = sorted(
            table_normal_down, key=lambda in_thr_out: in_thr_out[0], reverse=True
        )
        self.table_onefail_up = sorted(
            table_onefail_up, key=lambda in_thr_out: in_thr_out[0], reverse=True
        )
        self.table_onefail_down = sorted(
            table_onefail_down, key=lambda in_thr_out: in_thr_out[0], reverse=True
        )
        self.table = self.table_normal_up
        self.compare_fsc_value = 0
        self.last_out = None
        self.accelerate = 1
        self.dead_fans = 0

    def run(self, value, ctx):
        mini = 0
        dead_fans = len(ctx["dead_fans"])

        if self.table == None:
            self.table = self.table_normal_up
        if self.compare_fsc_value == None:
            self.compare_fsc_value = value
            self.accelerate = 1
        elif value > self.compare_fsc_value:
            self.accelerate = 1
        elif value < self.compare_fsc_value:
            self.accelerate = 0

        if self.accelerate == 1 and dead_fans == 0:
            self.table = self.table_normal_up
        elif self.accelerate == 0 and dead_fans == 0:
            self.table = self.table_normal_down
        elif self.accelerate == 1 and dead_fans == 1:
            self.table = self.table_onefail_up
        elif self.accelerate == 0 and dead_fans == 1:
            self.table = self.table_onefail_down

        if self.accelerate:
            Logger.debug(" accelerate(up) table {0}".format(self.table))
        else:
            Logger.debug(" accelerate(down) table {0}".format(self.table))

        for (in_thr, out) in self.table:
            mini = out
            if self.last_out == None:
                self.last_out = out
            if value >= in_thr:
                self.compare_fsc_value = value
                if self.accelerate:  # ascending
                    Logger.debug(
                        " LINEAR pwmout max([%.0f,%.0f]) temp(%.2f)"
                        % (out, self.last_out, value)
                    )
                    self.last_out = max([out, self.last_out])
                else:  # descending
                    Logger.debug(
                        " LINEAR pwmout min[(%.0f,%.0f]) temp(%.2f)"
                        % (out, self.last_out, value)
                    )
                    self.last_out = min([out, self.last_out])
                return self.last_out

        self.compare_fsc_value = value
        if self.accelerate:  # ascending
            Logger.debug(
                " LINEAR pwmout max([%.0f,%.0f]) temp(%.2f)"
                % (mini, self.last_out, value)
            )
            self.last_out = max([mini, self.last_out])
        else:  # descending
            Logger.debug(
                " LINEAR pwmout min([%.0f,%.0f]) temp(%.2f)"
                % (mini, self.last_out, value)
            )
            self.last_out = min([mini, self.last_out])
        return self.last_out
