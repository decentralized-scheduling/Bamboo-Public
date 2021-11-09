import os
import subprocess
import sys

supported_ccalgs = [
    "QCC",
    "ORDERED_LOCK",
    "BASIC_SCHED",
    "BAMBOO",
    "SILO",
    "TICTOC",
    "WAIT_DIE",
    "NO_WAIT",
]

threadcnts = ["1", "2", "4", "8", "16", "24", "32"]

def get_algs():
    r = []
    for a in sys.argv:
        if a in supported_ccalgs:
            r.append(a)
    return r

def config(params):
    # turn off abort regulation for them
    if params.get("CC_ALG") in ["QCC", "ORDERED_LOCK", "BASIC_SCHED"]:
        params["ABORT_BUFFER_SIZE"] = "1"

    with open("../config-std.h") as fin, open("../config.h", "w") as fout:
        lines = (l.strip() for l in fin)
        for l in lines:
            tokens = l.split(None, 2)
            if len(tokens) == 3:
                if tokens[1] in params:
                    tokens[2] = str(params[tokens[1]])
            fout.write(" ".join(tokens) + '\n')


def ycsb(theta=0.99, read=0.5, write=0.5, nr_reqs=16, nr_threads=1, alg="QCC"):
    theta = str(theta)
    read = str(read)
    write = str(write)
    nr_reqs = str(nr_reqs)
    nr_threads = str(nr_threads)
    params = {
        "WORKLOAD": "YCSB",
        "SYNTH_TABLE_SIZE": "(100 * 1024 * 1024)",
        "ZIPF_THETA": theta,
        "READ_PERC": read,
        "WRITE_PERC": write,
        "REQ_PER_QUERY": nr_reqs,
        "THREAD_CNT": nr_threads,
        "CC_ALG": alg,
    }

    config(params)
    exp = "ycsb-%s-%s-%s-%s-%s-%s" % (theta, read, write, nr_reqs, nr_threads, alg)
    return exp


def tpcc(nr_wh=1, perc_payment=0.5, nr_threads=1, alg="QCC"):
    nr_wh = str(nr_wh)
    perc_payment = str(perc_payment)
    nr_threads = str(nr_threads)
    params = {
        "WORKLOAD": "TPCC",
        "NUM_WH": nr_wh,
        "PERC_PAYMENT": perc_payment,
        "THREAD_CNT": nr_threads,
        "CC_ALG": alg,
    }
    config(params)
    exp = "tpcc-%s-%s-%s-%s" % (nr_wh, perc_payment, nr_threads, alg)
    return exp


def run_exp(exp, nr_threads):
    nr_threads = str(nr_threads)
    # max 64 accesses according to config-std.h
    ret = os.system("make --no-print-directory -C .. QCC_WORKERS=%s QCC_MAX_ACCESSES=64 -B -j16 > /dev/null 2>&1" % (nr_threads))
    if (ret != 0):
        print("%s compile fails returns %s" % {exp, str(ret)}, flush=True)
    else:
        result = subprocess.check_output(["../rundb"], stderr=subprocess.STDOUT).decode("utf-8")
        if "PASS" in result:
            result = result.split("\n")
            result = [r.strip() for r in result]
            print("+++", exp, "+++", flush=True)
            for l in result:
                if "summary!" in l:
                    ll = l.strip().split(" ", 1)
                    print(ll[1], flush=True)
        else:
            print("%s fails" % (exp), flush=True)
    os.system("make --no-print-directory -C .. -j16 clean &> /dev/null")
