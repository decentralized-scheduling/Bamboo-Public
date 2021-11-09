from exp import *

# tpcc 16 thds, 0.5 payment 0.5 new_order

ccalgs = get_algs()
nr_wh = ["32", "16", "8", "4", "2", "1"]
for alg in ccalgs:
    for nr in nr_wh :
        exp = tpcc(nr, 0.5, 16, alg)
        run_exp(exp, 16)

