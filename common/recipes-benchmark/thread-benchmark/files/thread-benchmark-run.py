#!/usr/bin/python3
import subprocess
import sys
import time

from cpu_monitor import CPUProfiler

base_cpu_utilization = "20"
if len(sys.argv) > 1:
    base_cpu_utilization = sys.argv[1]
iterations = "1000"
if len(sys.argv) > 2:
    iterations = sys.argv[2]

with CPUProfiler() as p:
    subprocess.check_call(
        ["/usr/local/bin/thread-benchmark", "base", iterations, base_cpu_utilization]
    )
time.sleep(1)
with CPUProfiler() as p:
    subprocess.check_call(
        ["/usr/local/bin/thread-benchmark", "fork", iterations, base_cpu_utilization]
    )
time.sleep(1)
with CPUProfiler() as p:
    subprocess.check_call(
        ["/usr/local/bin/thread-benchmark", "pthread", iterations, base_cpu_utilization]
    )
