from exp import *

# tpcc linear wh, 0.5 payment 0.5 new_order

ccalgs = get_algs()
for alg in ccalgs:
    for nr_threads in threadcnts:
        exp = tpcc(nr_threads, 0.5, nr_threads, alg)
        run_exp(exp, nr_threads)

