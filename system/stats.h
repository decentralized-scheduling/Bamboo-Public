#pragma once

#define TMP_METRICS(x, y) \
  x(double, time_wait) x(double, time_man) x(double, time_index)
#define ALL_METRICS(x, y, z) \
  y(uint64_t, txn_cnt) y(uint64_t, abort_cnt) y(uint64_t, user_abort_cnt) \
  x(double, run_time) x(double, time_abort) x(double, time_cleanup) \
  x(double, time_query) x(double, time_get_latch) x(double, time_get_cs) \
  x(double, time_copy) x(double, time_retire_latch) x(double, time_retire_cs) \
  x(double, time_release_latch) x(double, time_release_cs) x(double, time_semaphore_cs) \
  x(double, time_commit) y(uint64_t, time_ts_alloc) y(uint64_t, wait_cnt) \
  y(uint64_t, latency) y(uint64_t, commit_latency) y(uint64_t, abort_length) \
  y(uint64_t, cascading_abort_times) z(uint64_t, max_abort_length) \
  y(uint64_t, txn_cnt_long) y(uint64_t, abort_cnt_long) y(uint64_t, cascading_abort_cnt) \
  y(uint64_t, lock_acquire_cnt) y(uint64_t, lock_directly_cnt) \
  TMP_METRICS(x, y)
#define DECLARE_VAR(tpe, name) tpe name;
#define INIT_VAR(tpe, name) name = 0;
#define INIT_TOTAL_VAR(tpe, name) tpe total_##name = 0;
#define SUM_UP_STATS(tpe, name) total_##name += _stats[tid]->name;
#define MAX_STATS(tpe, name) total_##name = max(total_##name, _stats[tid]->name);
#define STR(x) #x
#define XSTR(x) STR(x)
#define STR_X(tpe, name) XSTR(name)
#define VAL_X(tpe, name) total_##name / BILLION
#define VAL_Y(tpe, name) total_##name
#define PRINT_STAT_X(tpe, name) \
  std::cout << STR_X(tpe, name) << "= " << VAL_X(tpe, name) << ", ";
#define PRINT_STAT_Y(tpe, name) \
  std::cout << STR_X(tpe, name) << "= " << VAL_Y(tpe, name) << ", ";
#define WRITE_STAT_X(tpe, name) \
  outf << STR_X(tpe, name) << "= " << VAL_X(tpe, name) << ", ";
#define WRITE_STAT_Y(tpe, name) \
  outf << STR_X(tpe, name) << "= " << VAL_Y(tpe, name) << ", ";

class Stats_thd {
 public:
  void init(uint64_t thd_id);
  void clear();

  char _pad2[CL_SIZE];
  ALL_METRICS(DECLARE_VAR, DECLARE_VAR, DECLARE_VAR)
  uint64_t * all_debug1;
  uint64_t * all_debug2;
  char _pad[CL_SIZE];
};

class Stats_tmp {
 public:
  void init();
  void clear();
  double time_man;
  double time_index;
  double time_wait;
  char _pad[CL_SIZE - sizeof(double)*3];
};

class Stats {
 public:
  // PER THREAD statistics
  Stats_thd ** _stats;
  // stats are first written to tmp_stats, if the txn successfully commits,
  // copy the values in tmp_stats to _stats
  Stats_tmp ** tmp_stats;

  // GLOBAL statistics
  double dl_detect_time;
  double dl_wait_time;
  uint64_t cycle_detect;
  uint64_t deadlock;

  void init();
  void init(uint64_t thread_id);
  void clear(uint64_t tid);
  void add_debug(uint64_t thd_id, uint64_t value, uint32_t select);
  void commit(uint64_t thd_id);
  void abort(uint64_t thd_id);
  void print(uint64_t _time);
  void print_lat_distr();
};

// From Cicada / MICA

class Stat_latency {
 public:
  Stat_latency() { reset(); }

  void reset() { memset(this, 0, sizeof(Stat_latency)); }

  void update(uint64_t us) {
    if (us < 128)
      bin0_[us]++;
    else if (us < 384)
      bin1_[(us - 128) / 2]++;
    else if (us < 896)
      bin2_[(us - 384) / 4]++;
    else if (us < 1920)
      bin3_[(us - 896) / 8]++;
    else if (us < 3968)
      bin4_[(us - 1920) / 16]++;
    else if (us < 8064)
      bin5_[(us - 3968) / 32]++;
    else if (us < 16256)
      bin6_[(us - 8064) / 64]++;
    else if (us < 32640)
      bin7_[(us - 16256) / 128]++;
    else if (us < 65408) // max precision 65 ms..
      bin8_[(us - 32640) / 256]++;
    else {
      bin9_++; // 65408
    }
    // max..
    if (us > max_) max_ = us;
  }

  Stat_latency& operator+=(const Stat_latency& o) {
    uint64_t i;
    for (i = 0; i < 128; i++) bin0_[i] += o.bin0_[i];
    for (i = 0; i < 128; i++) bin1_[i] += o.bin1_[i];
    for (i = 0; i < 128; i++) bin2_[i] += o.bin2_[i];
    for (i = 0; i < 128; i++) bin3_[i] += o.bin3_[i];
    for (i = 0; i < 128; i++) bin4_[i] += o.bin4_[i];
    for (i = 0; i < 128; i++) bin5_[i] += o.bin5_[i];
    for (i = 0; i < 128; i++) bin6_[i] += o.bin6_[i];
    for (i = 0; i < 128; i++) bin7_[i] += o.bin7_[i];
    for (i = 0; i < 128; i++) bin8_[i] += o.bin8_[i];
    bin9_ += o.bin9_;
    if (max_ < o.max_) max_ = o.max_;
    return *this;
  }

  uint64_t count() const {
    uint64_t count = 0;
    uint64_t i;
    for (i = 0; i < 128; i++) count += bin0_[i];
    for (i = 0; i < 128; i++) count += bin1_[i];
    for (i = 0; i < 128; i++) count += bin2_[i];
    for (i = 0; i < 128; i++) count += bin3_[i];
    for (i = 0; i < 128; i++) count += bin4_[i];
    for (i = 0; i < 128; i++) count += bin5_[i];
    for (i = 0; i < 128; i++) count += bin6_[i];
    for (i = 0; i < 128; i++) count += bin7_[i];
    for (i = 0; i < 128; i++) count += bin8_[i];
    count += bin9_;
    return count;
  }

  uint64_t sum() const {
    uint64_t sum = 0;
    uint64_t i;
    for (i = 0; i < 128; i++) sum += bin0_[i] * (0 + i * 1);
    for (i = 0; i < 128; i++) sum += bin1_[i] * (128 + i * 2);
    for (i = 0; i < 128; i++) sum += bin2_[i] * (384 + i * 4);
    for (i = 0; i < 128; i++) sum += bin3_[i] * (896 + i * 8);
    for (i = 0; i < 128; i++) sum += bin4_[i] * (1920 + i * 16);
    for (i = 0; i < 128; i++) sum += bin5_[i] * (3968 + i * 32);
    for (i = 0; i < 128; i++) sum += bin6_[i] * (8064 + i * 64);
    for (i = 0; i < 128; i++) sum += bin7_[i] * (16256 + i * 128);
    for (i = 0; i < 128; i++) sum += bin8_[i] * (32640 + i * 256);
    sum += bin9_ * 65408;
    return sum;
  }

  uint64_t avg() const { return sum() / std::max(uint64_t(1), count()); }

  double avg_f() const {
    return static_cast<double>(sum()) /
           static_cast<double>(std::max(uint64_t(1), count()));
  }

  uint64_t min() const {
    uint64_t i;
    for (i = 0; i < 128; i++)
      if (bin0_[i] != 0) return 0 + i * 1;
    for (i = 0; i < 128; i++)
      if (bin1_[i] != 0) return 128 + i * 2;
    for (i = 0; i < 128; i++)
      if (bin2_[i] != 0) return 384 + i * 4;
    for (i = 0; i < 128; i++)
      if (bin3_[i] != 0) return 896 + i * 8;
    for (i = 0; i < 128; i++)
      if (bin4_[i] != 0) return 1920 + i * 16;
    for (i = 0; i < 128; i++)
      if (bin5_[i] != 0) return 3968 + i * 32;
    for (i = 0; i < 128; i++)
      if (bin6_[i] != 0) return 8064 + i * 64;
    for (i = 0; i < 128; i++)
      if (bin7_[i] != 0) return 16256 + i * 128;
    for (i = 0; i < 128; i++)
      if (bin8_[i] != 0) return 32640 + i * 256;

    return 65408;
  }

  uint64_t max() const {
    return max_;
  }

  uint64_t perc(double p) const {
    uint64_t i;
    int64_t thres = static_cast<int64_t>(p * static_cast<double>(count()));
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin0_[i])) < 0) return 0 + i * 1;
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin1_[i])) < 0) return 128 + i * 2;
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin2_[i])) < 0) return 384 + i * 4;
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin3_[i])) < 0) return 896 + i * 8;
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin4_[i])) < 0) return 1920 + i * 16;
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin5_[i])) < 0) return 3968 + i * 32;
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin6_[i])) < 0) return 8064 + i * 64;
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin7_[i])) < 0) return 16256 + i * 128;
    for (i = 0; i < 128; i++)
      if ((thres -= static_cast<int64_t>(bin8_[i])) < 0) return 32640 + i * 256;

    return 65408;
  }

 private:
  // [0, 128) us
  uint64_t bin0_[128];
  // [128, 384) us
  uint64_t bin1_[128];
  // [384, 896) us
  uint64_t bin2_[128];
  // [896, 1920) us
  uint64_t bin3_[128];
  // [1920, 3968) us
  uint64_t bin4_[128];
  // [3968, 8064) us
  uint64_t bin5_[128];
  // [8064, 16256] us
  uint64_t bin6_[128];
  // [16256, 32640] us
  uint64_t bin7_[128];
  // [32640, 65408] us
  uint64_t bin8_[128];
  // [65408, inf) us
  uint64_t bin9_;
  uint64_t max_;
};
