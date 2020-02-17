cp -r config_ycsb_synthetic.h config.h
rm debug.out

wl="YCSB"
alg=WOUND_WAIT
threads=32
cnt=100000 #0000
penalty=50000
zipf=0.99
synthetic=false
table_size="1024*1024*20"
profile="true"
req=16
spin="true"
on=1
dynamic="true"
hs=0
pos=TOP
fhs="WR"
shs="WR"
read_ratio=0.9
phs="false"
think=0
ordered="false"
ww_starv_free="false"

timeout 500 python test.py WW_STARV_FREE=${ww_starv_free} KEY_ORDER=$ordered THINKING_TIME=$think PRIORITIZE_HS=$phs READ_PERC=${read_ratio} NUM_HS=$hs FIRST_HS=$fhs POS_HS=$pos DEBUG_TMP="false" DYNAMIC_TS=$dynamic CLV_RETIRE_ON=$on SPINLOCK=$spin REQ_PER_QUERY=$req DEBUG_PROFILING=$profile SYNTH_TABLE_SIZE=${table_size} WORKLOAD=${wl} CC_ALG=$alg THREAD_CNT=$threads MAX_TXN_PER_PART=$cnt ABORT_PENALTY=$penalty ZIPF_THETA=$zipf SYNTHETIC_YCSB=$synthetic  |& tee -a debug.out
