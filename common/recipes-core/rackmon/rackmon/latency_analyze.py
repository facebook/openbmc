#!/usr/bin/python3
import sys

import matplotlib.pyplot as plt
import numpy as np
from matplotlib import colors

if len(sys.argv) < 2:
    print("USAGE: %s FILE [PLOTFILE]")
    sys.exit(1)
data_file = sys.argv[1]
image_file = None
if len(sys.argv) >= 3:
    image_file = sys.argv[2].replace(".png", "")


def plot(values, dest):
    n_bins = 1000
    fig, axs = plt.subplots(1, 1, tight_layout=True)
    plt.xlabel("Latency per modbus command (ms)")
    N, bins, patches = axs.hist(values, bins=n_bins)
    fracs = N / N.max()
    norm = colors.Normalize(fracs.min(), fracs.max())
    for thisfrac, thispatch in zip(fracs, patches):
        color = plt.cm.viridis(norm(thisfrac))
        thispatch.set_facecolor(color)
    fig.set_size_inches(20, 10)
    fig.savefig(dest, dpi=1000)


with open(data_file) as f:
    raw = filter(lambda x: "LATENCY_PROFILE" in x, f.read().splitlines())

    def caller_time_tuple(line):
        arr = line.split()
        comp = arr[8].split(":")
        if comp[-1] == "":
            comp.pop(-1)
        comp_str = ":".join(comp)
        return (comp_str, float(arr[-1]) / 1000.0)  # US to MS

    data = [caller_time_tuple(line.strip()) for line in raw]

data_by_col = {}
for d in data:
    if d[0] not in data_by_col:
        data_by_col[d[0]] = []
    data_by_col[d[0]].append(d[1])

print("entity,min(ms),median(ms),P90(ms),P99(ms),max(ms)")
for data_col in data_by_col:
    row = [data_col]
    data = data_by_col[data_col]
    row.append("{:.1f}".format(np.min(data)))
    row.append("{:.1f}".format(np.median(data)))
    row.append("{:.1f}".format(np.percentile(data, 90)))
    row.append("{:.1f}".format(np.percentile(data, 99)))
    row.append("{:.1f}".format(np.max(data)))
    print(",".join(row))
    if image_file:
        plot(data, image_file + data_col + ".png")
