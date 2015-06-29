/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Milos Prvulovic
                  Smruti Sarangi

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

/*
 * This launches the ESESC simulator environment with an ideal memory
 */
#define WARMUP_ARRAY_SIZE 262144
#include <sys/types.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "nanassert.h"

#include "QEMUReader.h"
#include "BootLoader.h"
#include "Transporter.h" 
#ifdef ENABLE_NBSD
#include "MemRequest.h"
void meminterface_start_snoop_req(uint64_t addr, bool inv, uint16_t coreid, bool dcache, void *_mreq) {
  MemRequest *mreq = (MemRequest *)_mreq;

  //MSG("@%lld snoop %s 0x%lx %d %s",globalClock, mreq->getCurrMem()->getName(), addr, mreq->getAction(), inv?"inv":"wb");
  mreq->convert2SetStateAck(ma_setInvalid,true); // WARNING: if it was a miss, ma_setShared
  mreq->getCurrMem()->doSetStateAck(mreq);
}
#endif

int64_t checkpoint_id;

static void *simu_thread(void *) {
  BootLoader::boot();
  BootLoader::report("done");

  BootLoader::unboot();
  BootLoader::unplug();
  
#ifdef ESESC_LIVE
  Transporter::send_fast("cp_done", "");
#endif

  printf("Live: Sim prcess done (killing itself %ld, %d)\n", checkpoint_id, getpid());
  kill(getpid(),SIGTERM); // suicide time

  return 0;
}

extern "C" void start_esesc(char * host_adr, int portno, int cpid, int force_warmup, int genwarm, uint64_t * live_warmup_addr, bool * live_warmup_st, uint64_t live_warmup_cnt, int dlc) {
  checkpoint_id = cpid;
  
#ifdef ESESC_LIVE
  Transporter::connect_to_server(host_adr, portno);
  Transporter::send_fast("cp_start", "%d,%d", cpid, getpid());
  printf("LiveSim Checkpoint thread starting CPID:%d PID:%d\n", cpid, getpid());
#endif
  // TODO: call a method (new) to set QEMUReader::started = true
  //
  // TODO: Create fake arguments using the esescfile
  int argc = 1;
  const char *argv[3];
  argv[0] = "esesc";
  argv[1] = 0;

  QEMUReader::setStarted();

  BootLoader::plug(argc, argv);
  BootLoader::plugSocket(cpid, force_warmup, genwarm, live_warmup_cnt);

  pthread_attr_t attr;
  pthread_attr_init(&attr);

  size_t stacksize = 1024*1024;
  pthread_attr_setstacksize(&attr, stacksize);

  pthread_t ptid;

  if (pthread_create(&ptid, &attr, simu_thread, (void *)0) != 0) {
    MSG("ERROR: pthread create failed for simu_thread");
    exit(-2);
  }

  //loading live warmup
  if(dlc > 0) {
    for(int i = 0; i < live_warmup_cnt && i < WARMUP_ARRAY_SIZE; i++) {
      if(live_warmup_st[i]) {
        QEMUReader_queue_inst(0, live_warmup_addr[i], 0, iSALU_ST, LREG_R0, LREG_R0, LREG_InvalidOutput);
      } else {
        QEMUReader_queue_inst(0, live_warmup_addr[i], 0, iLALU_LD, LREG_R0, LREG_R0, LREG_InvalidOutput);
      }
    }
  }
}
