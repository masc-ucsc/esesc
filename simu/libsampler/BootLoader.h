/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <sys/time.h>

#include "nanassert.h"
#include "PowerModel.h"

class BootLoader {
 private:
  static char *reportFile;
  static timeval stTime;
  static PowerModel  *pwrmodel; 
  static bool doPower;

  static void check();

 protected:
  static void plugEmulInterfaces();
  static void plugSimuInterfaces();
  static void createEmulInterface(const char *section, FlowID fid=0);
  static void createSimuInterface(const char *section, FlowID i);

 public:
  static EmuSampler *getSampler(const char *section, const char *keyword, EmulInterface *eint, FlowID fid);

  static void plug(int argc, const char **argv);
  static void boot();
  static void report(const char *str);
  static void unboot();
  static void unplug();

  // Dump statistics while the program is still running
  static void reportOnTheFly(const char *file=0); //eka, to be removed.
  static void startReportOnTheFly();
  static void stopReportOnTheFly();

  static PowerModel *getPowerModelPtr() { return pwrmodel; }
};

#endif
