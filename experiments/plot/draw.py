#!/usr/bin/python3
import sys
import matplotlib
import matplotlib.pyplot as plt

matplotlib.rcParams['font.family'] = 'Noto Serif'
fig = plt.figure(constrained_layout=True, figsize=(9.75, 3))
fontsize = 18
ticksize = 14

# 332288, 88CCEE, 44AA99, 117733, 999933, DDCC77, CC6677, 882255, AA4499

algs = {
    "QCC":           ["DecentSched", "#ff7f00", "s", "solid"],
    "BASIC_SCHED":   ["C-Sched",     "#e7298a", "x", "dashdot"],
    "ORDERED_LOCK":  ["Ord-Lock",    "#9f7a8c", "d", "dashdot"],
    "SILO":          ["Silo",        "#4daf4a", "o", "dotted"],
    "TICTOC":        ["TicToc",      "#66c2a5", "^", "dotted"],
    "BAMBOO":        ["Bamboo",      "#76a4db", "*", "dashed"],
    "WAIT_DIE":      ["Wait-Die",    "#667788", "+", "dashed"],
}

threads = [1, 4, 8, 16, 24, 32]

nplots = 3
gs = fig.add_gridspec(1, nplots)
ax = [fig.add_subplot(gs[:, i:i+1]) for i in range(nplots)]

exp = sys.argv[1]

throughputs = {}
arates = {}
latencies = {}


with open(exp + ".out", "r") as f:
    x = []
    y = []
    last = None
    while True:
        output = [f.readline().strip() for _ in range(3)]
        if "" in output:
            break
        data = [o.replace("+", "").replace(" ", "").replace(",", " ").replace("=", " ").replace(":", " ") for o in output]
        conf = data[0].split("-")
        if conf[0] == "tpcc":
            nr_threads = int(conf[3])
            alg = conf[4]
        else:
            nr_threads = int(conf[5])
            alg = conf[6]
        if alg not in throughputs or alg not in arates:
            throughputs[alg] = [[], []]
            arates[alg] = [[], []]
            latencies[alg] = [[], []]

        if nr_threads in threads:
            throughputs[alg][0].append(nr_threads)
            arates[alg][0].append(nr_threads)
            latencies[alg][0].append(nr_threads)

            val = data[1].split()
            throughputs[alg][1].append(float(val[1]))
            arates[alg][1].append(float(val[7]) * 100) # percentage

            val2 = data[2].split()
            latencies[alg][1].append(int(val2[11]) / 1000) # ms

# report the best of [SILO, TICTOC], [BAMBOO, WAIT_DIE]
combine = [["SILO", "TICTOC"], ["BAMBOO", "WAIT_DIE"]]
for i in range(len(threads)):
    for c in combine:
        arates[c[0]][1][i] = arates[c[0]][1][i] if throughputs[c[0]][1][i] > throughputs[c[1]][1][i] else arates[c[1]][1][i]
        latencies[c[0]][1][i] = latencies[c[0]][1][i] if throughputs[c[0]][1][i] > throughputs[c[1]][1][i] else latencies[c[1]][1][i]
        throughputs[c[0]][1][i] = max(throughputs[c[0]][1][i], throughputs[c[1]][1][i])

algs["SILO"][0] = "OCC"
algs["BAMBOO"][0] = "2PL"

for alg in algs:
    if alg == "TICTOC" or alg == "WAIT_DIE":
        continue
    ax[0].plot(threads, throughputs[alg][1], label=algs[alg][0], zorder=3, linewidth=1.25, marker=algs[alg][2], color=algs[alg][1], markersize=6, linestyle=algs[alg][3])
    ax[1].plot(threads, latencies[alg][1], label=algs[alg][0], zorder=3, linewidth=1.25, marker=algs[alg][2], color=algs[alg][1], markersize=6, linestyle=algs[alg][3])
    ax[2].plot(threads, arates[alg][1], label=algs[alg][0], zorder=3, linewidth=1.25, marker=algs[alg][2], color=algs[alg][1], markersize=6, linestyle=algs[alg][3])

for i in range(nplots):
    ax[i].grid(True, axis="both", linestyle="--", lw=0.5, color="#555555", zorder=0)
    ax[i].set_xticks(threads)
    ax[i].tick_params(labelsize=ticksize)
    ax[i].set_xlabel("# of threads", fontsize=fontsize)

ax[0].set_title("Throughput (Mtxn/s)", fontsize=fontsize)
ax[1].set_title("P99 latency (ms)", fontsize=fontsize)
ax[2].set_title("Abort ratio (%)", fontsize=fontsize)

ax[0].set_ylim(bottom=0)
ax[2].set_ylim(bottom=-5, top=100)

if exp == "ycsb-1":
    ax[0].set_ylim(top=0.99)
elif exp == "ycsb-2":
    ax[0].set_ylim(top=0.17)
elif exp == "ycsb-4":
    ax[1].set_ylim(bottom=-0.0025, top=0.06)

#elif exp == "tpcc-1":
#    ax[1].set_ylim(0, 50)
#elif exp == "tpcc-3":
#    ax[1].set_ylim(0, 25)
#elif exp == "tpcc-4":
#    ax[2].set_ylim(0, 0.5)
#    ax[1].set_ylim(0, 25)


if "tpcc" in exp:
    ax[0].set_ylim(0, 3.4)
    ax[1].set_ylim(bottom=-0.05, top=0.37)

handles, labels = ax[0].get_legend_handles_labels()
fig.legend(handles, labels, loc='upper center', fontsize=fontsize-1, ncol=7, bbox_to_anchor=(0.5, 1.2), handletextpad=0.2, borderpad=0.2)#, columnspacing=0.5)

fig.savefig(exp + ".pdf", bbox_inches="tight", pad_inches=0.03, format='pdf')

##fig.text(0.55, -0.06, "Number of Threads", fontsize=fontsize, ha="center")
#
## plot
#plt.close(fig)
