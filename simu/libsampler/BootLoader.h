// Contributed by Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <sys/time.h>

#include "PowerModel.h"
#include "nanassert.h"

class BootLoader {
private:
  static char *      reportFile;
  static timeval     stTime;
  static PowerModel *pwrmodel;
  static bool        doPower;

  static void check();

protected:
  static void plugEmulInterfaces();
  static void plugSimuInterfaces();
  static void createEmulInterface(const char *section, FlowID fid = 0);
  static void createSimuInterface(const char *section, FlowID i);

public:
  static int64_t sample_count;

  static EmuSampler *getSampler(const char *section, const char *keyword, EmulInterface *eint, FlowID fid);

  static void plug(int argc, const char **argv);
  static void plugSocket(int64_t cpid, int64_t fwu, int64_t gw, uint64_t lwcnt);
  static void boot();
  static void report(const char *str);
  static void reportSample();
  static void unboot();
  static void unplug();

  // Dump statistics while the program is still running
  static void reportOnTheFly(const char *file = 0); // eka, to be removed.
  static void startReportOnTheFly();
  static void stopReportOnTheFly();

  static PowerModel *getPowerModelPtr() {
    return pwrmodel;
  }
};

#endif
