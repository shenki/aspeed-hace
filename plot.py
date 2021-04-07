#!/usr/bin/python3
#
# Copyright 2021 Joel Stanley <joel@jms.id.au>, IBM Corp.
# SPDX-License-Identifier: GPL-2.0 OR Apache-2.0
#
# This program benchmarks the aspeed_hace test program against sha512sum with a
# varity of input file sizes, as defined in the list 'sizes' and produces a
# graph of the result. Should be run with this patch applied to the system's
# device tree:
#
#     --- a/arch/arm/boot/dts/aspeed-ast2600-evb.dts
#    +++ b/arch/arm/boot/dts/aspeed-ast2600-evb.dts
#    @@ -21,6 +21,17 @@ memory@80000000 {
#                    device_type = "memory";
#                    reg = <0x80000000 0x80000000>;
#            };
#    +
#    +       reserved-memory {
#    +               #address-cells = <1>;
#    +               #size-cells = <1>;
#    +               ranges;
#    +
#    +               hacks: framebuffer@90000000 {
#    +                       no-map;
#    +                       reg = <0x90000000 0x08000000>; /* 128M */
#    +               };
#    +       };
#     };
#
# Tested using Debian bullseye on an ast2600evb

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import subprocess
import re

sizes = ['8b', '16b', '8kB', '16kB', '32kB', '1MB', '8MB', '16MB', '64MB']
cpu = []
hace = []


def get_time(cmd):
    print("Running", " ".join(cmd))
    out = subprocess.run(cmd, capture_output=True, shell=True)
    try:
        m, s = out.stderr.split()[1].split(b'm')
        m = int(m)
        s = float(s[:-1])
    except Exception as e:
        print(out.stderr)
        print(cmd)
        print(e)
        import sys
        sys.exit(1)
    return m * 60 + s

def do_hace(size):
    return get_time(["time sudo ./aspeed_hace /tmp/test"])

def do_cpu(size):
    return get_time(["time sha512sum /tmp/test"])


for size in sizes:
    bs = re.sub(r'\d+', '', size)
    count = size.strip(bs)
    print("Creating {}{} file /tmp/test".format(count, bs))
    if bs == 'b':
        bs = '1'
    subprocess.run(["dd", "status=none", "if=/dev/random", "of=/tmp/test", "bs={}".format(bs) , "count={}".format(count)])

    hace.append(do_hace(size))
    cpu.append(do_cpu(size))

    print()


x = np.arange(len(sizes)) # the label locations
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
rects1 = ax.bar(x - width/2, cpu, width, label="CPU")
rects2 = ax.bar(x + width/2, hace, width, label="HACE")

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Runtime (seconds)')
ax.set_title('SHA512 hashing performance')
ax.set_xticks(x)
ax.set_xticklabels(sizes)
ax.legend()

def autolabel(rects):
    """Attach a text label above each bar in *rects*, displaying its height."""
    for rect in rects:
        height = rect.get_height()
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')


autolabel(rects1)
autolabel(rects2)

fig.tight_layout()

#plt.show()
plt.savefig("plot.png")
