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

from fsc_util import Logger, clamp


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

        if self.last_out is None:
            self.last_out = sorted(self.table)[0][1]

        for (in_thr, out) in self.table:
            mini = out
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


class IndependentPID:
    def __init__(self, pTable):
        self.last_error = 0
        self.I = 0
        self.kp = pTable.get("kp", 0.0)
        self.ki = pTable.get("ki", 0.0)
        self.kd = pTable.get("kd", 0.0)
        self.minval = pTable.get("setpoint", 0.0) - pTable.get(
            "negative_hysteresis", 0.0
        )
        self.maxval = pTable.get("setpoint", 0.0) + pTable.get(
            "positive_hysteresis", 0.0
        )
        self.minimum_outval = pTable.get(
            "minimum_outval", 0.001
        )  # set minimum_outval to 0.001 if not defined
        self.maximum_outval = 100
        self.pos_slew = pTable.get("positive_slew_rate", 0.0)
        self.neg_slew = pTable.get("negative_slew_rate", 0.0)
        self.last_out = 0
        self.last_value = 0

    # PI control only, D term is not used
    def run(self, value, ctx):
        dt = ctx["dt"]

        # if measured value is lower than the limitation, run the PID controller to reduce the fan speed and increase the temperature (positive error and negative output)
        if value < self.minval:
            error = self.minval - value
            # if the measured value in last time is higher than self. minval, reset the I term to be zero and recalculate the output
            if self.last_value > self.minval:
                self.I = 0
        # if measured value is higher than self.maxval, run the PID controller to increase the fan speed and reduce the temperature (negative error and positive output)
        elif value > self.maxval:
            error = self.maxval - value
            # if the measured value in last time is lower than self. maxval, reset the I term to be zero and recalculate the output
            if self.last_value < self.maxval:
                self.I = 0
        else:  # just retutn last_out if measured value within minval & maxval
            self.last_out = clamp(
                self.last_out, self.minimum_outval, self.maximum_outval
            )
            return self.last_out

        # calculate out value
        self.I = self.I + error * dt
        out = self.kp * error + self.ki * self.I

        # calculate slew rate
        max_slew_out = 0
        min_slew_out = 0
        if self.pos_slew > 0:
            max_slew_out = self.last_out + (self.pos_slew * dt)
            out = min(out, max_slew_out)

        if self.neg_slew < 0:
            min_slew_out = self.last_out + (self.neg_slew * dt)
            out = max(out, min_slew_out)

        Logger.info(
            "New PID debug: kp=%.2f, ki=%.2f, kd=%.2f, maxval=%.2f, minval=%.2f"
            % (self.kp, self.ki, self.kd, self.maxval, self.minval)
        )
        Logger.info(
            "New PID debug: pos_slew=%.2f, neg_slew=%.2f, minimum_outval=%.2f"
            % (self.pos_slew, self.neg_slew, self.minimum_outval)
        )
        Logger.info(
            "New PID debug: last_out=%.2f, last_error=%.2f, last_value=%.2f"
            % (self.last_out, self.last_error, self.last_value)
        )
        Logger.info(
            "New PID debug: out=%.2f, error=%.2f, value=%.2f" % (out, error, value)
        )
        Logger.info(
            "New PID debug: I=%.2f, dt=%.2f, max_slew_out=%.2f, min_slew_out=%.2f"
            % (self.I, dt, max_slew_out, min_slew_out)
        )

        self.last_out = clamp(out, self.minimum_outval, self.maximum_outval)
        self.last_error = error
        self.last_value = value

        return self.last_out


class Feedforward:
    def __init__(self, pTable):
        self.kf = pTable.get("kf", 0.0)

    def run(self, value, ctx):
        return self.kf * value
