# DecentSched-Bamboo

The repository is forked from Bamboo (DBx1000). The main purpose of this repository is to provide
the integration of DecentSched in a real benchmark system.

Please note that the main implementation of DecentSched is in the [main repository]
(https://github.com/decentralized-scheduling/DecentSched).

## Implementation

We implemented DecentSched as a concurrency control flavor in Bamboo's codebase (codename: `QCC`),
along with centralized scheduling (`BASIC_SCHED`) and ordered locking (`ORD_LOCK`). They
can be used as a regular concurrency control protocol, just like the existing ones in the codebase.

## Experiments

The experiment scripts used for the research paper is included in `experiments` directory.
The `run.sh` is a one-click script that will run all the experiments in one shot.
Experiment setup was set based on the specification of CloudLab's c6420 machine (single node).

Before experiments are run, please make sure all submodules are checked out.

For YCSB experiments, Fig. 5(a)-(d) correspond to `ycsb-1.py` to `ycsb-4.py` accordingly.
For TPC-C-NP experiments, Fig. 6(a) and 6(b) correspond to `tpcc-1.py` and `tpcc-2.py`. Fig. 6(c)
is produced by `tpcc-6.py` and Fig. 6(d) is produced by `tpcc-5.py`.

Once all the results are collected, they can be copied to the `plot` directory and the `draw.sh`
script will generate all the plots.
