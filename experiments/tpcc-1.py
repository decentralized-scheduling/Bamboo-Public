from exp import *

# tpcc 8 wh, 0.5 payment 0.5 new_order

ccalgs = get_algs()
for alg in ccalgs:
    for nr_threads in threadcnts:
        exp = tpcc(8, 0.5, nr_threads, alg)
        run_exp(exp, nr_threads)

