#ifndef CODEPROFILE_H
#define CODEPROFILE_H

#include "GStats.h"
#include "estl.h"

class CodeProfile : public GStats {
private:
  class ProfEntry {
  public:
    ProfEntry() {
      n   = 0;
      sum_cpi = 0;
      sum_wt  = 0;
      sum_et  = 0;
      sum_flush  = 0;
    }
    double n;
    double sum_cpi;
    double sum_wt;
    double sum_et;
    uint64_t sum_flush;
  };

  typedef HASH_MAP<uint64_t, ProfEntry> Prof;

  Prof prof;

  double last_nCommitted;
  double last_clockTicks;
  double nTotal;

protected:
public:
  CodeProfile(const char *format,...);

  void sample(const uint64_t pc, const double nCommitted, const double clockTicks, double wt, double et, bool flush);

  double  getDouble() const;
  int64_t getSamples() const;

  void reportValue() const;
  void reportBinValue() const;
  void reportScheme() const;

  void flushValue();
};

#endif
