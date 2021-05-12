#!/usr/bin/python3

import argparse
import fcntl
import glob
import os
import re
import subprocess
import sys
import time


class LockFile:
    def __init__(self, path):
        if os.path.exists(path):
            self.file = open(path, "w")  # noqa: P201
        else:
            self.file = open(path, "+w")  # noqa: P201

    def __deinit__(self):
        self.file.close()

    def __enter__(self):
        if self.file.writable():
            fcntl.lockf(self.file, fcntl.LOCK_EX)
        return self

    def __exit__(self, exc_type=None, exc_value=None, traceback=None):
        self.file.flush()
        os.fsync(self.file.fileno())
        if self.file.writable():
            fcntl.lockf(self.file, fcntl.LOCK_UN)
        self.file.close()
        if exc_type is not None:
            return False
        return True


class SolitonBeamLock(LockFile):
    def __init__(self):
        LockFile.__init__(self, "/tmp/modbus_dynamo_solitonbeam.lock")


def _modbus(cmd, timeout):
    resp_len_words = int(cmd[-2:], 16)
    # each words is 2 bytes, add 5 byte header.
    resp_len = 2 * resp_len_words + 5
    act_cmd = ["/usr/local/bin/modbuscmd", "-t", str(timeout), "-x", str(resp_len), cmd]
    out = subprocess.check_output(act_cmd).decode().split()
    if out[0] != "Response:":
        raise ValueError("Unexpected response")
    return [int(i, 16) for i in out[1:]]


def modbus(cmd, timeout):
    with SolitonBeamLock() as _:
        return _modbus(cmd, timeout)


def dynamo_loop(data_size, addrs, timeout):
    rem_field = "03007a00"
    for addr in addrs:
        cmd = addr + rem_field + "%02x" % (data_size)
        modbus(cmd, timeout)


def sv(svc_list, cmd):
    failed = []
    for svc in svc_list:
        if svc == "":
            continue
        try:
            subprocess.check_call(["sv", cmd, svc])
            if svc == "fscd" and cmd == "stop":
                subprocess.check_call(["wdtcli", "set-timeout", "3600"])
        except Exception:
            failed.append(svc)
    if len(failed) > 0 and len(svc_list) > 0:
        print(",".join(failed) + " services failed command: " + cmd)


def sv_stop(svc_list):
    sv(svc_list, "stop")
    if len(svc_list) > 0:
        time.sleep(20)


def sv_start(svc_list):
    sv(svc_list, "start")
    # some services do a lot of work at start (eg: restapi)
    # this can really mess up the next experiment's CPU
    # reading. so, sleep here.
    if len(svc_list) > 0:
        time.sleep(120)


def rackmonstatus():
    rx = re.compile(
        r"PSU addr (\S+) - crc errors: (\d+), timeouts: (\d+), baud rate: (\d+)"
    )
    ret = {}
    out = subprocess.check_output(["rackmonstatus"]).decode().splitlines()
    for line in out:
        m = rx.search(line)
        if m:
            addr = m.group(1)
            crc = int(m.group(2))
            timeouts = int(m.group(3))
            baud = int(m.group(4))
            ret[addr] = {"crc": crc, "timeouts": timeouts, "baud": baud}
    return ret


def rackmon_run_stats(before, after):
    removed = [addr for addr in before.keys() if addr not in after.keys()]
    added = [addr for addr in after.keys() if addr not in after.keys()]
    if len(removed) > 0:
        print("ERROR: %s PSUs were taked out during experiment" % (",".join(removed)))
    if len(added) > 0:
        print("ERROR: %s PSUs were added during experiment" % (",".join(added)))
    for psu in before:
        if psu not in after:
            continue
        crcs = after[psu]["crc"] - before[psu]["crc"]
        timeouts = after[psu]["timeouts"] - before[psu]["timeouts"]
        baud_before = before[psu]["baud"]
        baud_after = after[psu]["baud"]
        return baud_after - baud_before, crcs, timeouts


class CPUProfiler:
    last_run = None

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
        CPUProfiler.last_run = self.timestamp(full=True)

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
        self.last_run = diff
        return diff


def run_experiment_loop(iters, size, psus, interval, timeout=500):
    total_time = 0.0
    overruns = 0
    with CPUProfiler() as _:
        for _ in range(0, iters):
            bts = time.time()
            dynamo_loop(size, psus, timeout)
            runtime_sec = time.time() - bts
            total_time += runtime_sec
            if interval > 0:
                if runtime_sec < interval:
                    time.sleep(interval - runtime_sec)
                else:
                    overruns += 1
    exp_run = CPUProfiler.last_run
    return exp_run, 1000.0 * total_time / float(len(psus) * iters), overruns


DEFAULT_NUM_ITERS = 1000
DEFAULT_DATA_SIZE = 1
DEFAULT_LOGFILE = "./modbus_results.txt"

parser = argparse.ArgumentParser(
    description="Modbus experment runner",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
parser.add_argument(
    "--experiment",
    choices=[
        "latency_distribution",
        "latency_cpu_mean",
    ],
    dest="experiment",
    default="latency_cpu_mean",
    help="The type of experment to run (TODO describe)",
)
parser.add_argument(
    "--psus",
    dest="psus",
    default="a4,a5,a6,b4,b5,b6",
    help="The PSUs to run the experiment on",
)
parser.add_argument(
    "--iterations",
    dest="num_iters",
    type=int,
    default=1000,
    help="The number of iterations",
)
parser.add_argument(
    "--size", dest="size", default=1, help="The data size read from the PSU (words)"
)
parser.add_argument(
    "--output",
    dest="output",
    type=argparse.FileType("w"),
    default=sys.stdout,
    help="The output for the data file",
)
parser.add_argument(
    "--silent-period",
    dest="silent",
    type=int,
    default=180,
    help="Study base CPU utilization for this period to compare with test run",
)
parser.add_argument(
    "--interval",
    dest="interval",
    type=int,
    default=0,
    help="Sleep time between iterations",
)
parser.add_argument(
    "--timeout",
    dest="timeout",
    type=int,
    default=500,
    help="Timeout used for each try of the modbus command",
)
parser.add_argument(
    "--stop",
    dest="stop",
    default="",
    help="stop the following services before experiment",
)
args = parser.parse_args()

stop_svc = list(filter(None, args.stop.split(",")))
sv_stop(stop_svc)
before_status = rackmonstatus()

if args.experiment == "latency_distribution":
    # Delete all /var/log/messages and restart rsyslog.
    for log in glob.glob("/var/log/messages*"):
        print("Removing:", log)
        os.remove(log)
    subprocess.check_call(["/etc/init.d/syslog", "restart"])
    time.sleep(5)
    total_time = 0

    # Run the experiment
    run_experiment_loop(
        args.num_iters, args.size, args.psus.split(","), args.interval, args.timeout
    )
    time.sleep(5)
    logs = sorted(glob.glob("/var/log/messages*"))
    for log in logs:
        with open(log) as inf:
            for line in inf:
                if "LATENCY" in line:
                    args.output.write(line)
elif args.experiment == "latency_cpu_mean":
    with CPUProfiler() as _:
        time.sleep(args.silent)
    base_run = CPUProfiler.last_run
    # Run the experiment
    exp_run, exp_latency, overruns = run_experiment_loop(
        args.num_iters, args.size, args.psus.split(","), args.interval, args.timeout
    )
    base_util = base_run["utilization"]
    exp_util = exp_run["utilization"]
    exp_time = exp_run["time"]
    num_messages = float(len(args.psus.split(",")) * args.num_iters)
    avg_latency = exp_time * 1000.0 / num_messages
    # This will be true when interval > 0.
    if avg_latency > exp_latency:
        avg_latency = exp_latency
    after_status = rackmonstatus()
    bc, crc, t = rackmon_run_stats(before_status, after_status)
    headers = [
        "Base Utilization (%%)",
        "Exp Utilization (%%)",
        "AverageLatency (ms)",
        "Baud change",
        "CRC Errors",
        "Timeouts",
        "Overruns",
    ]
    print(",".join(headers))
    print(
        "%.1f,%.1f,%.2f,%d,%d,%d,%d"
        % (base_util, exp_util, avg_latency, bc, crc, t, overruns)
    )

sv_start(stop_svc)
