#!/usr/bin/python3

import os
import time


class CPUProfiler:
    def discover_hz(self):
        return int(os.sysconf(os.sysconf_names["SC_CLK_TCK"]))

    def get_proc(self):
        ret = {}
        with open("/proc/stat") as f:
            for line in f.read().splitlines():
                arr = line.strip().split()
                key = arr.pop(0)
                ret[key] = arr
        return ret

    def get_proc_cpu(self):
        keys = [
            "user",
            "nice",
            "system",
            "idle",
            "iowait",
            "irq",
            "softirq",
            "steal",
            "guest",
            "guest_nice",
        ]
        info = [int(x) for x in self.get_proc()["cpu"]]
        ret = dict(zip(keys, info))
        ret["total_idle"] = ret["idle"] + ret["iowait"]
        ret["total_busy"] = (
            ret["user"]
            + ret["nice"]
            + ret["system"]
            + ret["irq"]
            + ret["softirq"]
            + ret["steal"]
        )
        return ret

    def get_diff(self, old, new):
        ret = {}
        for key in old:
            ret[key] = new[key] - old[key]
        return ret

    def __init__(self):
        self.hz = self.discover_hz()
        self.cur = self.get_proc_cpu()
        self.prev = self.cur
        self.init = time.time()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        print(self.timestamp(full=True))

    def timestamp(self, full=False):
        new = self.get_proc_cpu()
        tick = time.time() - self.init
        if full:
            p = self.cur
        else:
            p = self.prev
        diff = self.get_diff(p, new)
        for k in diff:
            diff[k] = float(diff[k]) / float(self.hz)
        self.prev = new
        total_time = diff["total_busy"] + diff["total_idle"]
        diff["utilization"] = 100.0 * diff["total_busy"] / total_time
        diff["time"] = tick
        return diff


if __name__ == "__main__":
    with CPUProfiler() as p:
        time.sleep(300)
