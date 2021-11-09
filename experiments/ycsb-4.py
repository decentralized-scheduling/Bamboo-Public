from exp import *

# ycsb 0 zipf 0.5 read 0.5 write 16pt

ccalgs = get_algs()
for alg in ccalgs:
    for nr_threads in threadcnts:
        exp = ycsb(0, 0.5, 0.5, 16, nr_threads, alg)
        run_exp(exp, nr_threads)

