//#if 0
#include "GPUThreadManager.h"

extern   uint64_t numThreads;
extern    int64_t maxSamplingThreads;

extern   uint64_t GPUFlowsAlive;
extern   bool*    GPUFlowStatus;

extern   uint64_t oldqemuid;
extern   uint64_t newqemuid;

extern   uint32_t traceSize;
extern   bool     unifiedCPUGPUmem;
extern   bool*    istsfifoBlocked;
extern   bool     rabbit_threads_executing;
extern ThreadBlockStatus *block;
extern div_type branch_div_mech;

extern   GStatsCntr*  totalPTXinst_T;
extern   GStatsCntr*  totalPTXinst_R;
extern   GStatsCntr*  totalPTXmemAccess_Shared;
extern   GStatsCntr*  totalPTXmemAccess_Global;
extern   GStatsCntr*  totalPTXmemAccess_Rest;

extern uint64_t GPUReader_translate_d2h(AddrType addr_ptr);
extern uint64_t GPUReader_translate_shared(AddrType addr_ptr, uint32_t blockid, uint32_t warpid);
extern uint32_t roundUp(uint32_t numToRound, uint32_t multiple);
extern uint32_t roundDown(uint32_t numToRound, uint32_t multiple);

extern uint64_t loopcounter;
extern uint64_t relaunchcounter;

extern uint64_t memref_interval;

bool GPUThreadManager::Timing_serial_branchdiv(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid) {
  bool atleast_oneSM_alive                 = false;
  uint64_t threads_in_current_warp_done    = 0;
  uint64_t threads_in_current_warp_barrier = 0;
  uint64_t timing_threads_complete         = 0;                  // USEME
  int64_t total_timing_threads             = maxSamplingThreads;

  if ((int64_t) total_timing_threads == -1) {
    total_timing_threads = numThreads;
  }

  uint32_t smid                            = 0;
  uint64_t warpid                          = 0;
  uint64_t glofid                          = 0;
  int64_t  active_thread                   = 0;
  uint32_t trace_currentbbid               = 0;
  uint32_t trace_nextbbid                  = 0;
  uint32_t trace_skipinstcount             = 0;
  uint32_t trace_branchtaken               = 0;
  uint32_t trace_unifiedBlockID            = 0;
  //uint32_t trace_warpid;

  CUDAKernel* kernel   = kernels[current_CUDA_kernel];
  traceSize            = kernel->tracesize;
  uint32_t tracestart  = (12 + kernel->predicates);    // Fixed.
  RAWInstType insn     = 0;
  //AddrType    addr     = 0;
  //DataType    data     = 0;
  uint32_t    addr     = 0;
  uint32_t    data     = 0;
  uint32_t    pc       = 0;
  char        op       = 0;
  uint64_t    icount   = 1;
  bool memoryInstfound = false;
  bool changewarpnow   = false;
  bool tsfifoblocked = false;

  bool fresh_relaunch = true;
  bool printnow = false;

  gsampler->startTiming(glofid); //startTiming calls emul->startTiming, which does not really use the flowID, so we are okay

  do { //Outermost Loop
    smid                    = 0;
    atleast_oneSM_alive     = false;
    timing_threads_complete = 0;

    for (smid = 0; smid < numSM; smid++) { //Loop L1
      glofid          = mapLocalID(smid);
      memoryInstfound = false;
      changewarpnow   = false;

      if ((SM[smid].smstatus == SMalive))  { //Loop L2

        I(SM[smid].timing_warps > 0); //Otherwise the status should be something else
        warpid = SM[smid].getCurrentWarp();

        if (fresh_relaunch == true) {
          if (printnow)
            MSG("Warp %llu : ################################ FRESH RELAUNCH ###################################",warpid);
          fresh_relaunch = false;
        }

        if (SM[smid].warp_status[warpid] == waiting_barrier) {
          uint32_t blockid = SM[smid].warp_block_map[warpid];
          uint32_t compl_th = 0;
          for (uint32_t w_per_b = 0; w_per_b<block[blockid].warps.size(); w_per_b++) {
            uint32_t localwid = block[blockid].warps[w_per_b];
            compl_th +=  SM[smid].warp_complete[localwid];
          }

          if (block[blockid].checkThreadCount(blockSize,compl_th) == true) {
            //if (printnow)
            fprintf (stderr, "SM[%d] Warp [%d] Have to clear warps in block %d :", (int) smid, (int) warpid, (int) blockid);
            block[blockid].clearBarrier();
            uint32_t localwid = 0;
            for (uint32_t w_per_b = 0; w_per_b<block[blockid].warps.size(); w_per_b++) {
              localwid = block[blockid].warps[w_per_b];
              //if (printnow) IS(fprintf(stderr, "%d ,", localwid));
              SM[smid].warp_status[localwid]         = relaunchReady;
              SM[smid].warp_last_active_pe[localwid] = 0;
              SM[smid].warp_blocked[localwid]        = 0;
              //FIXME: Should I be doing anything else here, like say unpausing a particular warp?
              for (uint32_t spid = 0; spid < numSP; spid++) {
                if ((localwid < SM[smid].threads_per_pe[spid].size()) && (SM[smid].thread_status_per_pe[spid][localwid] != invalid)) {
                  SM[smid].thread_status_per_pe[spid][localwid] = block_done;
                }
              }
            }

            //FIXME: Next two statements redundant??
            SM[smid].warp_status[warpid]      = relaunchReady;
            SM[smid].warp_blocked[warpid]     = 0;
          } else {
            I(0);
            // Something is seriously incorrect!!
            // Should never get here
          }
        }
        else if (SM[smid].warp_status[warpid] == waiting_memory) {
          SM[smid].warp_status[warpid] = traceReady;
        }
        else if (SM[smid].warp_status[warpid] == waiting_TSFIFO) {
          SM[smid].warp_status[warpid] = traceReady;

        }
        else if (SM[smid].warp_status[warpid] == relaunchReady) {
          // No more instructions to insert from this warp.
          // Mark the SM as done and wait for the kernel to be respawned for more instructions
          SM[smid].smstatus = SMdone;
        }
        else if (SM[smid].warp_status[warpid] == warpComplete) {
          I(0); //Just to see if we ever reach here.
          //Move to the next warp.
          //Not sure if this needs to be there.
        }


        if (SM[smid].warp_status[warpid] == traceReady) { //Loop L3
          // This state indicates there is data from at least some threads to insert into the queues
          if (printnow)
            IS(MSG("********* Starting SM [%d] *********", smid));

          threads_in_current_warp_done    = 0;
          threads_in_current_warp_barrier = 0;

          bool start_halfway              = false;
          if (SM[smid].warp_last_active_pe[warpid]) {
            start_halfway                 = true;
          }

          tsfifoblocked = false;

          for (int32_t pe_id = SM[smid].warp_last_active_pe[warpid]; ((pe_id < (int32_t)numSP) || (start_halfway)) && !(tsfifoblocked); pe_id++) { //Loop L4
            if (warpid < SM[smid].threads_per_pe[pe_id].size()) { // Loop L5

              memref_interval += 1;
              if (SM[smid].thread_status_per_pe[pe_id][warpid] == paused_on_a_memaccess) {
                I(0);
                SM[smid].thread_status_per_pe[pe_id][warpid] = running; //Change it to running or whatever.
              }
              else if (SM[smid].thread_status_per_pe[pe_id][warpid] == paused_on_a_barrier) {
                threads_in_current_warp_barrier++;
              }

              if (SM[smid].thread_status_per_pe[pe_id][warpid] == running) { // Loop L6


                if (SM[smid].divergence_data_valid[warpid] == false) { // Loop L7
                  //If we are switching to the warp for the first time,
                  //build the divergent threads set.
                  SM[smid].warp_divergence[warpid].divergent_bbs.clear();
                  SM[smid].warp_divergence[warpid].bb_mapped_threads.clear();

                  for (int32_t l_pe_id = 0; (l_pe_id < (int32_t)numSP); l_pe_id++) { // Loop L8
                    uint64_t l_active_thread        = SM[smid].threads_per_pe[l_pe_id][warpid];
                    uint32_t l_currentbb = h_trace_qemu[(l_active_thread*traceSize)+1];
                    uint32_t l_pausedbb = h_trace_qemu[(l_active_thread*traceSize)+4];
                    if ((l_pausedbb != 1) && (l_currentbb != 0)) {
                      SM[smid].warp_divergence[warpid].divergent_bbs.insert(l_currentbb);
                      SM[smid].warp_divergence[warpid].bb_mapped_threads.insert(std::pair<uint32_t,uint64_t>(l_currentbb,l_active_thread));
                    } else {
                      if (printnow)
                        MSG ("Warp %llu : Ignoring thread %llu which goes to paused basic block %d", warpid, l_active_thread, l_currentbb);
                    }
                  } // End Loop L8
#if 1
                  if (printnow) {
                    cout << "Warp " <<warpid<<" : ";
                    cout << "SM[" << smid << "] , warp [" <<warpid<<"] contains new divergence stats " << endl;
                    warp_div_data tmp_div_data = SM[smid].warp_divergence[warpid];
                    while (!tmp_div_data.divergent_bbs.empty()) {
                      printDivergentList(smid,warpid,*tmp_div_data.divergent_bbs.begin());
                      tmp_div_data.divergent_bbs.erase(tmp_div_data.divergent_bbs.begin());
                    }
                  }
#endif
                  SM[smid].divergence_data_valid[warpid] = true;
                  SM[smid].unmasked_bb[warpid] = *(SM[smid].warp_divergence[warpid].divergent_bbs.begin());

                } //End Loop L7

                active_thread        = SM[smid].threads_per_pe[pe_id][warpid];
                I(active_thread >= 0);
                trace_nextbbid       = h_trace_qemu[(active_thread*traceSize)+0];
                trace_currentbbid    = h_trace_qemu[(active_thread*traceSize)+1];
                trace_skipinstcount  = h_trace_qemu[(active_thread*traceSize)+2];
                trace_branchtaken    = h_trace_qemu[(active_thread*traceSize)+3];
                trace_unifiedBlockID = h_trace_qemu[(active_thread*traceSize)+5];

                uint32_t numbbinst   = kernel->bb[trace_currentbbid].number_of_insts;
                uint32_t instoffset  = 0;
                uint32_t traceoffset = 0;

                if (trace_currentbbid == 0 ) { //Loop L9
                  SM[smid].thread_status_per_pe[pe_id][warpid] = block_done; //or should it be execution_done?
                  threads_in_current_warp_done++;
                } else { //Else Loop L9
                  if (trace_currentbbid == SM[smid].unmasked_bb[warpid]){ //Loop L10
                    if (printnow)
                      MSG("Warp %llu : Current BBID is now %u", warpid, trace_currentbbid);
                    if (kernel->isBarrierBlock(trace_currentbbid)) { // Loop L11
                      uint32_t blockid = SM[smid].warp_block_map[warpid];

                      if (printnow)
                        MSG ("Warp %d : BARRIER: SM %d PE %d Thread %d Block %d blocked on BB[%d], will resume from BB[%d]",(int) warpid, (int)glofid, (int)pe_id, (int)active_thread, blockid, (int)trace_currentbbid, (int) trace_nextbbid);

                      block[blockid].incthreadcount();
                      SM[smid].thread_status_per_pe[pe_id][warpid] = paused_on_a_barrier;

                      h_trace_qemu[active_thread*traceSize+4]=1;  //Pause the current thread.
                      threads_in_current_warp_barrier++;          //Increment the block_barrier_count by one.
                      threads_in_current_warp_done++;             //Incremented when the thread is done, or hits a barrier
                      SM[smid].warp_blocked[warpid]++;

                      //  Insert a RALU instruction
                      //  FIXME-Alamelu:

                      uint32_t compl_th = 0;
                      for (uint32_t w_per_b = 0; w_per_b<block[blockid].warps.size(); w_per_b++) {
                        uint32_t localwid = block[blockid].warps[w_per_b];
                        compl_th         +=  SM[smid].warp_complete[localwid];
                      }

                      if (block[blockid].checkThreadCount(blockSize,compl_th) == true) { //Loop L12
                        if (printnow)
                          fprintf (stderr, "Warp %llu : SM[%d] Warp [%d] Have to clear warps in block %d :", warpid, (int) smid, (int) warpid, (int) blockid);

                        block[blockid].clearBarrier();
                        uint32_t localwid = 0;
                        for (uint32_t w_per_b = 0; w_per_b<block[blockid].warps.size(); w_per_b++) {
                          localwid = block[blockid].warps[w_per_b];
                          //if (printnow) IS(fprintf(stderr, "%d ,", localwid));
                          SM[smid].warp_status[localwid]         = relaunchReady;
                          SM[smid].warp_last_active_pe[localwid] = 0;
                          SM[smid].warp_blocked[localwid]        = 0;
                          for (uint32_t spid = 0; spid < numSP; spid++) {
                            if ((localwid < SM[smid].threads_per_pe[spid].size()) && (SM[smid].thread_status_per_pe[spid][localwid] != invalid)) {
                              SM[smid].thread_status_per_pe[spid][localwid] = block_done;
                            }
                          }
                        }
                        // MSG(" ");
                        SM[smid].warp_blocked[warpid] = blockSize;
                      } // End Loop L12

                      //Since this thread has reached the barrier, we remove this thread from the divergent list.
                      removeFromDivergentList(smid,warpid,trace_currentbbid,active_thread);
                      if (SM[smid].warp_divergence[warpid].bb_mapped_threads.count(trace_currentbbid) == 0) { // Loop L13
                        if (switch2nextDivergentBB(smid, warpid) == true){
                          SM[smid].unmasked_bb[warpid] = *(SM[smid].warp_divergence[warpid].divergent_bbs.begin());
                          if (printnow)
                            MSG("Warp %llu : Switched to another BB from this barrier BB", warpid);
                        } else { //Else Loop L13
                          if (printnow)
                            cout << "Warp " <<warpid<<" : All divergent threads in this warp have now been executed." <<endl;
                        } // End Loop L13
                      }
                    } else { // Else Loop L11
                      // Not a barrier block
                      SM[smid].warp_last_active_pe[warpid] = 0;

                      if (!(kernel->isDummyBB(trace_currentbbid))) { //Loop L14
                        // Not a Dummy BBID
                        instoffset      = SM[smid].thread_instoffset_per_pe[pe_id][warpid];
                        traceoffset     = SM[smid].thread_traceoffset_per_pe[pe_id][warpid];
                        insn            = CUDAKernel::packID(trace_currentbbid, instoffset);
                        CudaInst *cinst = kernel->getInst(trace_currentbbid, instoffset);
                        pc              = cinst->pc;
                        op              = cinst->opkind;
                        CUDAMemType memaccesstype;

                        if (op == 0) {// ALU ops, Loop L15
                          addr = 0;

                        } else if (op == 3) {// Branches, Else IF loop L15
                          if (!(trace_branchtaken)) { // Loop L16
                            addr = 0;//Not Address of h_trace_qemu[(tid*traceSize+0)];
                          } else { // Else Loop 16
                            if (trace_nextbbid != 0) { //Loop L17
                              if (kernel->isDummyBB(trace_nextbbid)) { //Loop L18
                                uint16_t tempnextbb = trace_nextbbid+1;

                                while ((kernel->isDummyBB(tempnextbb)) && (tempnextbb <= kernel->getLastBB()) ) {
                                  tempnextbb++;
                                }

                                if (tempnextbb <= kernel->getLastBB()) {
                                  addr = kernel->getInst(tempnextbb,0)->pc;
                                } else {
                                  I(0);       //next blocks are all 0!
                                  addr = 0x0; //ERROR
                                }

                              } else { // Else Loop L18
                                addr = kernel->getInst(trace_nextbbid,0)->pc;
                              } // End Loop L18

                            } // End Loop L17
                          } // End Loop L16
                        } else {//Load Store, Else Loop L15
                          memaccesstype = kernel->bb[trace_currentbbid].insts[instoffset].memaccesstype;
                          if ((memaccesstype == ParamMem) || (memaccesstype == RegisterMem) || (memaccesstype == ConstantMem) || (memaccesstype == TextureMem)) {
                            addr = 1000; //FIXME: Constant MEM
                          } else {
                            addr = h_trace_qemu[active_thread*traceSize+tracestart+traceoffset];
                            traceoffset++;
                          }

                          if (addr) { // Loop L19
                            I(op == 1 || op == 2); //Sanity check;
                            if (memaccesstype == GlobalMem) { //Loop L20
                              //addr = GPUReader_translate_d2h(addr);
                              //addr = GPUReader_encode_rawaddr(addr, trace_unifiedBlockID, pe_id, false);
                              addr = GPUReader_encode_rawaddr(addr, trace_unifiedBlockID, warpid, pe_id, false);
#if TRACKGPU_MEMADDR
                              if ((pe_id == 1)) {
                                memdata <<active_thread<< ","<<type<<","<< hex << pc <<","<< hex << addr <<endl;
                              }
#endif
                            } else if (memaccesstype == SharedMem) { // ELse if loop L20
                              //addr = GPUReader_translate_shared(addr, trace_unifiedBlockID, warpid);
                              //addr = GPUReader_translate_shared(addr, trace_unifiedBlockID,0);
                              //addr = GPUReader_encode_rawaddr(addr, trace_unifiedBlockID, pe_id, true);
                              addr = GPUReader_encode_rawaddr(addr, trace_unifiedBlockID, warpid, pe_id, true);
#if TRACKGPU_MEMADDR
                              if ((pe_id == 1)) {
                                memdata <<active_thread<< ","<<type<<","<< hex << pc <<","<< hex << addr <<endl;
                              }
#endif
                            } //End Loop L20
                          }// End Loop L19
                        }// End Loop L15

                        data = 0;

                        icount = 1;

                        uint64_t addr_64bit = pe_id;
                        addr_64bit = addr_64bit << 16 |((SM[smid].getCurrentLocalWarp())*numSP);
                        addr_64bit = addr_64bit << 32 | addr;
#if 0
                        if ((op==2) || (op==1)){
                          MSG ("*****************************************************************");
                          MSG( "Size of AddrType = %d",sizeof(addr));
                          MSG( "Size of DataType = %d",sizeof(data));
                          MSG( "Addr has  = 0x%08X",addr);
                          MSG( "Data has  = 0x%08X",data);
                          MSG (" PE_ID is 0x%04X",pe_id);
                          MSG (" WarpID is 0x%04X",(SM[smid].getCurrentLocalWarp())*numSP);
                          MSG( "Addr_64bit (with pe and warp information) has  = 0x%016llx",addr_64bit);
                          MSG( "Addr_64bit (with addr) has  = 0x%016llX",addr_64bit);
                          MSG ("*****************************************************************");
                        }
#endif
												if ((pe_id == 0))
													gsampler->queue(insn,pc,addr_64bit,glofid,op,icount,env);

                        if (istsfifoBlocked[glofid]) { //Loop L21
                          if (printnow)
                          MSG("Warp %d :\t\t\t TSFIFO[%llu] BLOCKED, (SM[%u], Warp[%llu]). Moving to Next SM",(int) warpid,glofid,smid,warpid);
                          //MSG("Last Active PE                = %d", (int) pe_id);
                          gsampler->resumeThread(glofid);
                          istsfifoBlocked[glofid]              = false;
                          SM[smid].warp_status[warpid]         = waiting_TSFIFO;
                          SM[smid].warp_last_active_pe[warpid] = pe_id;
                          pe_id                                = numSP+1;           // move on to the next sm.
                          tsfifoblocked                        = true;
                          memref_interval -= 1;
                        } else { //Else Loop L21
                          if (printnow)
                            MSG("Warp %d : SM (%d) Thread %d (BB %d ) Adding inst [%d] to CPU %d, PE %d---->PC %x",(int) warpid, (int)smid, (int)active_thread, (int) trace_currentbbid, instoffset, (int) glofid, (int) pe_id, pc);
                          SM[smid].thread_instoffset_per_pe[pe_id][warpid]++;

                          totalPTXinst_T->add(icount);

                          if ((op == 1) ||(op == 2)) {  // Memory ops, Loop L22
                            if ((memaccesstype == SharedMem)) {
                              totalPTXmemAccess_Shared->inc();
                            } else if ((memaccesstype == GlobalMem)) {
                              totalPTXmemAccess_Global->inc();
                            } else {
                              totalPTXmemAccess_Rest->inc();
                            }
                            SM[smid].thread_traceoffset_per_pe[pe_id][warpid]++;
                            //MSG("Memory inst");
                            if (L1cache.checkAddr(addr,memref_interval) == false){
                              memoryInstfound = true;
                              //MSG("Miss on the L1 cache!!");
                            } else {
                              //MSG("Hit on the L1 cache!!");
                            }
                          }// End Loop L22

                          //if all the instructions in the block have been inserted.
                          if (SM[smid].thread_instoffset_per_pe[pe_id][warpid] >= (int32_t) numbbinst) { //Loop L23
                            threads_in_current_warp_done++;
                            removeFromDivergentList(smid,warpid,trace_currentbbid,active_thread);
                            if (SM[smid].warp_divergence[warpid].bb_mapped_threads.count(trace_currentbbid) == 0) { //Loop L24
                              if (switch2nextDivergentBB(smid, warpid) == true){
                                SM[smid].unmasked_bb[warpid] = *(SM[smid].warp_divergence[warpid].divergent_bbs.begin());
                              } else {
                                if (printnow)
                                  cout << "Warp " <<warpid<<" : All divergent threads in this warp have now been executed." <<endl;
                              }
                            }// End Loop L24

                            if (trace_nextbbid == 0) { // Loop L25
                              SM[smid].thread_status_per_pe[pe_id][warpid] = execution_done;                                                                                      // OK
                              h_trace_qemu[active_thread*traceSize+4]      = 1;                                                                                                   // Pause
                              h_trace_qemu[(active_thread*traceSize)+1]    = 0;                                                                                                   // Mark the currentbbid to 0 so that u don't keep inserting the instruction
                              SM[smid].timingthreadsComplete++;
                              //I(SM[smid].warp_status[warpid] != paused_on_a_barrier);
                              I(SM[smid].warp_status[warpid] != waiting_barrier);
                              SM[smid].warp_complete[warpid]++;
                              if (printnow)
                                MSG("Thread %d at SM[%d] PE [%d][%llu] BB [%d] Inst [%d] COMPLETE",(int)active_thread,(int)smid, (int)pe_id, warpid,trace_currentbbid, instoffset);

                              if (SM[smid].warp_complete[warpid] >= numSP) {
                                if (printnow)
                                  MSG("SM[%d] PE [%d] WARP [%d] COMPLETE.. Should change warp!!",(int)smid, (int)pe_id, (int)warpid);
                                SM[smid].warp_status[warpid]               = warpComplete;
                              }

                            } else {//Else Loop L25
                              SM[smid].thread_status_per_pe[pe_id][warpid] = block_done;
                              SM[smid].warp_last_active_pe[warpid]         = 0;
                            } // End Loop L25
                          }// End Loop L23
                        } //End Loop L21
                      }  else {// Else Loop L14
                        //Dummy BBID
                        threads_in_current_warp_done++;
                      }// End Loop L14

                    } // End Loop L11
                  } else {// Else Loop L10
                    //cout << "Warp " <<warpid<<" : ";
                    if (printnow)
                      cout << "Ignoring thread " << active_thread << " PE (" << pe_id << ") because it goes to BB "<< trace_currentbbid <<" and not " <<SM[smid].unmasked_bb[warpid] << "(Head of divergent bbs is " << *(SM[smid].warp_divergence[warpid].divergent_bbs.begin()) << " )" << endl;
                  } // End Loop L10
                } // End Loop L9

              } else {// Else Loop L6
                /*
                   if (SM[smid].thread_status_per_pe[pe_id][warpid] == running ) {
                   MSG ("Warp  %llu: Why am I here, in the (If thread_status != running) loop, Thread %llu is running",warpid, active_thread);
                   } else if (SM[smid].thread_status_per_pe[pe_id][warpid] == paused_on_a_memaccess) {
                   MSG ("Warp  %llu: Why am I here, in the (If thread_status != running) loop Thread %llu is paused_on_a_memaccess",warpid, active_thread);
                   } else if (SM[smid].thread_status_per_pe[pe_id][warpid] == paused_on_a_barrier) {
                   MSG ("Warp  %llu: Why am I here, in the (If thread_status != running) loop Thread %llu is paused_on_a_barrier",warpid, active_thread);
                   } else if (SM[smid].thread_status_per_pe[pe_id][warpid] == block_done) {
                   MSG ("Warp  %llu: Why am I here, in the (If thread_status != running) loop Thread %llu is block_done",warpid, active_thread);
                   } else if (SM[smid].thread_status_per_pe[pe_id][warpid] == invalid) {
                   MSG ("Warp  %llu: Why am I here, in the (If thread_status != running) loop Thread %llu is invalid",warpid, active_thread);
                   } else {
                   MSG ("Warp  %llu: Why am I here, in the (If thread_status != running) loop Thread %llu is weird!!!!",warpid, active_thread);
                   }
                   */
                //If thread_status != running or any other status that does not allow you to insert instructions
                if (likely(SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)) {
                  //fprintf(stderr,"*");
                  threads_in_current_warp_done++;
                }
              } // End Loop L6
            } //End Loop L5

            if ((pe_id == (int32_t)(numSP - 1)) && (start_halfway)) { //Loop L26
              //MSG("------>>>>>>>>> Reached here <<<<<<<<------[%d]",pe_id);
              SM[smid].warp_last_active_pe[warpid] = 0;
              pe_id                                = -1;
              start_halfway                        = false;
              threads_in_current_warp_done         = 0;
            } //End Loop L26

          } //End Loop L4

          //Now that I have attempted to insert instructions from all the pes in the SM,


          if (SM[smid].warp_blocked[warpid] > 0) { //Loop L27
            if ((SM[smid].warp_blocked[warpid]+SM[smid].warp_complete[warpid]) >= SM[smid].active_sps[warpid]) { //Loop L28
              if (printnow)
                MSG("Warp %llu : All threads in current warp blocked",warpid);
              SM[smid].warp_status[warpid] = waiting_barrier;
              if (printnow)
                MSG("Marking SM[%d] warp [%d] as wating_barrier", (int)smid, (int)warpid);
              if ((SM[smid].warp_blocked[warpid]+SM[smid].warp_complete[warpid]) >= blockSize) {
                SM[smid].warp_status[warpid]      = relaunchReady;
                SM[smid].warp_blocked[warpid]     = 0;
                if (printnow)
                  MSG("UnMarking SM[%d] warp [%d] as wating_barrier", (int)smid, (int)warpid);
              }
              changewarpnow   = true;
            } else if (threads_in_current_warp_done >= SM[smid].active_sps[warpid]) { //Else if loop L28
              SM[smid].warp_status[warpid] = relaunchReady;
              SM[smid].smstatus = SMdone;
              SM[smid].divergence_data_valid[warpid] = false;
            } // End Loop L28
          } else if (SM[smid].warp_complete[warpid] >= SM[smid].active_sps[warpid]) {//Else if loop L27
            if (printnow)
              MSG("All threads in current warp complete");
            SM[smid].warp_status[warpid] = warpComplete;
            changewarpnow   = true; //Move to the next warp.
            SM[smid].smstatus = SMdone;
            SM[smid].divergence_data_valid[warpid] = false;
          } else if (threads_in_current_warp_done >= SM[smid].active_sps[warpid]) {//Else if loop L27
            if (printnow)
              MSG("Warp %llu : All threads in current warp done",warpid);
            SM[smid].warp_status[warpid] = relaunchReady;
            SM[smid].smstatus = SMdone;
            SM[smid].divergence_data_valid[warpid] = false;
          } else {//Else loop L27
            if (printnow)
              MSG("Warp %llu : [%d] threads in current warp blocked\n [%d] threads in current warp complete\n [%d] threads in current warp done\n " ,warpid, (int)SM[smid].warp_blocked[warpid] ,(int)SM[smid].warp_complete[warpid] ,(int)threads_in_current_warp_done);
          }// End loop L27


          if (memoryInstfound || changewarpnow ) { // Loop L29
            //Move to the next warp.
            //Make sure that the threads in the current warp are paused
            //Switch to the next warp in the warpset which is not "Complete"
            //See if the new warp has instructions to insert. If yes, then good, otherwise mark SMstatus as SMDone, and be ready for relaunch
            //SM[smid].smstatus = SMdone;

            if (memoryInstfound) {
              //MSG("ENCOUNTERED MEMORY INSTRUCTION!!!!!!!! SHOULD BE SWITCHING TO THE NEXT WARP");
              SM[smid].warp_status[warpid] = waiting_memory;
            } else if (changewarpnow ) {
              I((SM[smid].warp_status[warpid] == warpComplete) || (SM[smid].warp_status[warpid] == relaunchReady) || (SM[smid].warp_status[warpid] == waiting_barrier));
            }

            bool warpSwitchsucc = switch2nextwarp(smid,h_trace_qemu);
            if (!warpSwitchsucc) { //Loop L30
              //if (printnow)
              //MSG("Switch2nextwarp returned false");
              SM[smid].smstatus = SMcomplete;
            } else { // Else Loop L30
              uint64_t newwarpid = 0;
              newwarpid = SM[smid].getCurrentWarp();
              if (   (SM[smid].warp_status[newwarpid] == traceReady)) { //Loop L31
                //if (printnow)
                //MSG("Yes,traceReady new warp has instructions to insert");
                SM[smid].smstatus = SMdone;

              } else if (SM[smid].warp_status[newwarpid] == waiting_barrier) {//Else if Loop L31
                //if (printnow)
                //MSG("Yes,new waiting_barrier warp has instructions to insert");
                //I(0);

              } else if (SM[smid].warp_status[newwarpid] == waiting_memory) {//Else if Loop L31
                //if (printnow)
                //MSG("Yes,new waiting_memory warp has instructions to insert");
                SM[smid].smstatus = SMalive;

              } else {//Else Loop L31
                switch (SM[smid].warp_status[newwarpid]) {
                  case waiting_TSFIFO:
                    //if (printnow)
                    MSG("1: No, new warp has NO instructions to insert, warpstatus is waiting_tsfifo");
                    break;
                  case relaunchReady:
                    //if (printnow)
                    MSG("2: No, new warp has NO instructions to insert, warpstatus is relaunchready");
                    break;
                  case warpComplete:
                    //if (printnow)
                    MSG("3: No, new warp has NO instructions to insert, warpstatus is warpComplete");
                    break;
                  default:
                    I(0);
                    break;
                }
                SM[smid].smstatus = SMdone;
              }// End Loop L31
            } // End Loop L30
            memoryInstfound = false;
            changewarpnow   = false;
            threads_in_current_warp_done  = 0;
          } // End Loop L29

        } //End Loop L3

        possiblyturnOFFSM(smid);

        if (SM[smid].smstatus == SMalive) {
          atleast_oneSM_alive = true;
        }

      }  else if (SM[smid].smstatus == SMdone ) { // Else if Loop L2
        //MSG("SM[%d] is SMDone",(int)smid);
        //Nothing for now.
        //possiblyturnOFFSM(smid);
      } else if (SM[smid].smstatus == SMcomplete) {// Else if Loop L2
        //MSG("SM[%d] is SMComplete",(int)smid);
        //Should already be turned off.
        //possiblyturnOFFSM(smid);
      } else if (SM[smid].smstatus == SMRabbit) {// Else if Loop L2
        //MSG("SM[%d] is SMRabbit",(int)smid);
        //Nothing for now.
        //possiblyturnOFFSM(smid);
      } // End Loop L2


      timing_threads_complete += SM[smid].timingthreadsComplete ;
    } //End Loop L1

  } while (atleast_oneSM_alive); // End of Outermost Loop

  gsampler->stop();

  if (printnow)
    IS(MSG("Number of Timing threads completed = %llu out of %llu\n\n\n",timing_threads_complete, total_timing_threads));

  if ((int64_t)timing_threads_complete < total_timing_threads){
    return true;
  } else {
    float ratio = static_cast<float>(numThreads)/static_cast<float>(total_timing_threads);
    gsampler->getGPUCycles(*qemuid,ratio);
    return false;
  }
}

void GPUThreadManager::removeFromDivergentList(uint64_t smid, uint64_t warpid, uint32_t bbid, uint64_t active_thread){
  typedef multimap<uint32_t, uint64_t>::iterator iter;

  //Remove from the map
  std::pair<iter, iter> iterpair = SM[smid].warp_divergence[warpid].bb_mapped_threads.equal_range(bbid);
  for (iter local_it = iterpair.first; local_it != iterpair.second; ++local_it) {
    if (local_it->second == active_thread) {
      SM[smid].warp_divergence[warpid].bb_mapped_threads.erase(local_it);
      break;
    }
  }
}

void GPUThreadManager::printDivergentList(uint64_t smid, uint64_t warpid, uint32_t bbid){
  fprintf(stderr,"Warp %llu : SM[%llu] contains BB %d (%d elems) : ",warpid,smid,bbid, SM[smid].warp_divergence[warpid].bb_mapped_threads.count(bbid));

  typedef multimap<uint32_t, uint64_t>::iterator iter;
  std::pair<iter, iter> iterpair = SM[smid].warp_divergence[warpid].bb_mapped_threads.equal_range(bbid);
  for (iter it=iterpair.first; it!=iterpair.second; ++it){
    fprintf(stderr," %llu",it->second);
  }
  fprintf(stderr,"\n");
}

bool GPUThreadManager::switch2nextDivergentBB(uint64_t smid, uint64_t warpid){
  bool returnvalue = false;

  //MSG("Warp %llu : Attempting to switch to the next divergent BB",warpid);
  do {

    //MSG("Warp %llu : Switching to the next divergent BB",warpid);
    SM[smid].warp_divergence[warpid].divergent_bbs.erase(SM[smid].warp_divergence[warpid].divergent_bbs.begin());

    if (SM[smid].warp_divergence[warpid].divergent_bbs.empty()) {
      //MSG("Warp %llu : The list of divergent BBs is empty. Which means that I should have inserted all the instructions",warpid);
      SM[smid].divergence_data_valid[warpid] = false;
      //MSG("Warp %llu : Did not switch to the next divergent BB",warpid);
      return false;
    }

    if (*SM[smid].warp_divergence[warpid].divergent_bbs.begin() != 0) {
      //MSG("Warp %llu : Now adding instructions that go to the next divergent BB (%u)",warpid,*SM[smid].warp_divergence[warpid].divergent_bbs.begin());
      returnvalue = true;
    }else{
      //MSG("Warp %llu : the next divergent BB is zero (%u), Skipping....",warpid,*SM[smid].warp_divergence[warpid].divergent_bbs.begin());
    }

  } while (returnvalue == false);


  //MSG("Warp (XXX) : Successful switch to the next divergent BB");
  return returnvalue;
}

//#endif
