#ifndef CODEPROFILE_H
#define CODEPROFILE_H

#include "GStats.h"
#include "estl.h"

class CodeProfile : public GStats {
private:
  class ProfEntry {
  public:
    ProfEntry() {
      n         = 0;
      sum_cpi   = 0;
      sum_wt    = 0;
      sum_et    = 0;
      sum_flush = 0;
      ldbr      = 0;
      sum_bp1_hit = 0;
      sum_bp2_hit = 0;
      sum_bp3_hit = 0;
      sum_bp1_miss = 0;
      sum_bp2_miss = 0;
      sum_bp3_miss = 0;
      sum_hit2_miss3 = 0;
      sum_hit3_miss2 = 0;
      sum_no_tl = 0;
      sum_late_tl = 0;
      sum_on_time_tl = 0;
      sum_trig_ld1_pred = 0;
      sum_trig_ld1_unpred = 0;
      sum_trig_ld2_pred = 0;
      sum_trig_ld2_unpred = 0;
    }
    double   n;
    double   sum_cpi;
    double   sum_wt;
    double   sum_et;
    uint64_t sum_flush;
    int      ldbr;
    uint64_t sum_bp1_hit;
    uint64_t sum_bp2_hit;
    uint64_t sum_bp3_hit;
    uint64_t sum_bp1_miss;
    uint64_t sum_bp2_miss;
    uint64_t sum_bp3_miss;
    uint64_t sum_hit2_miss3;
    uint64_t sum_hit3_miss2;
    uint64_t sum_no_tl;
    uint64_t sum_late_tl;
    uint64_t sum_on_time_tl;
    uint64_t sum_trig_ld1_pred;
    uint64_t sum_trig_ld1_unpred;
    uint64_t sum_trig_ld2_pred;
    uint64_t sum_trig_ld2_unpred;
    uint64_t sum_prefetch;
  };

  typedef HASH_MAP<uint64_t, ProfEntry> Prof;

  Prof prof;

  double last_nCommitted;
  double last_clockTicks;
  double nTotal;

protected:
public:
  CodeProfile(const char *format, ...);

  void sample(const uint64_t pc, const double nCommitted, const double clockTicks, double wt, double et, bool flush, bool prefetch, int ldbr = 0, bool bp1_miss = 0, bool bp2_miss = 0, bool bp3_miss = 0, bool bp1_hit = 0, bool bp2_hit = 0, bool bp3_hit = 0, bool hit2_miss3 = 0, bool hit3_miss2 = 0, bool tl1_pred = 0, bool tl1_unpred = 0, bool tl2_pred = 0, bool tl2_unpred = 0, int trig_ld_status = -1);

  double  getDouble() const;
  int64_t getSamples() const;

  void reportValue() const;
  void reportBinValue() const;
  void reportScheme() const;

  void flushValue();
};

#endif
