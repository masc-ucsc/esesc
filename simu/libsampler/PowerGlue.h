#ifndef POWERGLUE_H
#define POWERGLUE_H

/* License & includes {{{1 */
/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by 
                  Ehsan K.Ardestani

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <vector>
#include <map>
#include "Wrapper.h"
#include "TaskHandler.h"
#include "Bundle.h"



class PowerGlue {

  private:

    const char     *pwr_sec;
    ChipEnergyBundle *energyBundle;
    bool reFloorplan;
    uint32_t nStats;
    ofstream statsFile;




    GProcessor *getSimu(FlowID fid)              { return TaskHandler::getSimu(fid); };
    void initStatCounters();
    void closeStatCounters();
    void createCoreStats();
    void createCoreLayoutDescr();
    void createMemLayoutDescr();
    void createCoreStats(const char *section, uint32_t i);
    void createCoreDescr(const char *section, uint32_t i);
    void autoCreateCoreDescr(const char *section);
    void createMemStats();
    void createMemStats(const char *section, uint32_t i);
    void createMemObjStats(const char *temp_sec, const char *pname, const char *name); 
    char *getText(const char *format,...);
    char *replicateVar(const char *format, const char *s1, const char *s);
    uint32_t size(const char *str);
    char *getStr(const char *str, uint32_t i); 
    char *privateName(const string *name);
    void checkStatCounters(const char *section);
    void addVRecord(const char *sec, const char *str);


  public:

    PowerGlue();
    void createStatCounters();
    void dumpFlpDescr(uint32_t coreEIdx); 
    void plug(const char *section, ChipEnergyBundle *eBundle);
    void unplug();
};

#endif //POWERGLUE_H
