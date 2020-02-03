cp -r config_ycsb_synthetic.h config.h
rm debug.out

wl="YCSB"
alg="CLV"
threads=16
cnt=100000
penalty=0
zipf=0
synthetic="true"
table_size="1024*1024*20"
profile="true"
req=16
spin="true"
on=1
dynamic="true"
hs=1
pos="TOP"
#pos="MID"
#pos="BOT"
fhs="WR"
shs="WR"
read_ratio=1
phs="true"
phs="false"

for alg in CLV 
do
timeout 30 python test.py PRIORITIZE_HS=$phs READ_PERC=1 NUM_HS=$hs FIRST_HS=$fhs POS_HS=$pos DEBUG_TMP="false" DYNAMIC_TS=$dynamic CLV_RETIRE_ON=$on SPINLOCK=$spin REQ_PER_QUERY=$req DEBUG_PROFILING=$profile SYNTH_TABLE_SIZE=${table_size} WORKLOAD=${wl} CC_ALG=$alg THREAD_CNT=$threads MAX_TXN_PER_PART=$cnt ABORT_PENALTY=$penalty ZIPF_THETA=$zipf SYNTHETIC_YCSB=$synthetic  |& tee -a debug.out
done
#phs="false"

#timeout 30 python test.py PRIORITIZE_HS=$phs CLV_RETIRE_ON=$on SPINLOCK=$spin REQ_PER_QUERY=$req DEBUG_PROFILING=$profile SYNTH_TABLE_SIZE=${table_size} WORKLOAD=${wl} CC_ALG=$alg THREAD_CNT=$threads MAX_TXN_PER_PART=$cnt ABORT_PENALTY=$penalty ZIPF_THETA=$zipf SYNTHETIC_YCSB=$synthetic  |& tee -a debug.out