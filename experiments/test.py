from exp import *

ccalgs = ["BASIC_SCHED", "QCC", "ORDERED_LOCK"]
threadcnts = ["1", "4", "8", "16"]

for alg in ccalgs:
    for nr_threads in threadcnts:
        exp = tpcc(1, 0.5, nr_threads, alg)
        run_exp(exp, nr_threads)

