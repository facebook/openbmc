class PID:
    def __init__(self, setpoint, kp=0.0, ki=0.0, kd=0.0, minval=0.0, maxval=100.0):
        self.last_error = 0
        self.I = 0
        self.setpoint = setpoint
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.minval = minval
        self.maxval = maxval

    def run(self, value, dt):
        # don't accumulate into I term outside of operating range
        if value < self.minval:
            self.I = 0
            return None
        if value > self.maxval:
            self.I = 0
            return None
        error = self.setpoint - value
        self.I = self.I + error*dt
        D = (error - self.last_error) / dt
        out = self.kp * error + self.ki * self.I + self.kd * D
        self.last_error = error
        return out


# Threshold table
class TTable:
    def __init__(self, table):
        self.table = sorted(table,
                            key=lambda (in_thr, out): in_thr,
                            reverse=True)

    def run(self, value, dt):
        mini = 0
        for (in_thr, out) in self.table:
            mini = out
            if value >= in_thr:
                return out
        return mini
