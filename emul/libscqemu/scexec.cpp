//  Contributed by Jose Renau
//                 Ehsan K.Ardestani
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

#include "scexec.h"
#include <regex.h>

extern volatile int done;
extern bool thread_done[];

#define DEBUG2

Scexec::Scexec(FlowID _fid, void *syscall_mem, TranslationType ttype, ProgramBase *_prog, FILE *_fp, FILE *syscallTrace_fp)
:core(_fid, _prog)
{
  prog  = _prog;
  fid   = _fid;
  nInst = 0;
  nUops = 0;
  nHit  = 0;
  fp = _fp;
  firstIteration = true;
  vInst = 0;
  dmb_sy_flag = false;
  ldrex_block = false;

  core.reset();
  core.sync();

  core.setReg(LREG_SYSMEM, (uint64_t) syscall_mem);
  core.setSyscallTraceFile(syscallTrace_fp);

  rinst = new RAWDInst;

  crackInstThumb = new ThumbCrack;
  crackInstARM   = new ARMCrack;
#ifdef SPARC32
  crackInstSPARC = new SPARCCrack;
#endif

  if (ttype == ARM)
    crackInst = crackInstARM; //default to ARM
#ifdef SPARC32
  else if(ttype == SPARC32)
    crackInst = crackInstSPARC; 
#endif
  else
    crackInst = crackInstThumb; //default to Thumb

  core.setReg(LREG_TTYPE, ttype);
  core.setCrackInt(crackInst);

  crackInst->setPC(prog->get_reg(LREG_PC,ttype));

  for(uint8_t i = 0; i< ((ttype == SPARC32) ? 32 : 16); i++) {
    DataType d;
    if(ttype != SPARC32) {
      d = prog->get_reg(i+1, ttype); // for scqemumain use i
      core.setReg(i+1, d);
    }
#ifdef SPARC32
    else {
      d = prog->get_reg(i, ttype);
      core.setReg(i, d);
    }
#endif
#ifdef DEBUG2
    printf("reg[%d]=0X%x\n", i, d);
#endif
  }
}

void Scexec::exec_loop() {

  timeval st_time, end_time;
  gettimeofday(&st_time, NULL);

  while(nInst++ < 100000000 && !thread_done[fid]){ 
    fetch_crack();
    execute();
  }

  printf("<<<<<<<<<<<<<<<CPU fid %d Done>>>>>>>>>>>>>>>\n", fid);

  // commented out for the ProgramStandAlone
//  if(fid == 0) //FIXME: This is not always true for the end of execution of all apps.
//    done = 1;

  gettimeofday(&end_time, NULL);
  double t_msec= (end_time.tv_sec - st_time.tv_sec)*1000.0 + (end_time.tv_usec - st_time.tv_usec)/1000.0;
  std::cout << "Instruction Count: "     << nInst-1               << ", time (msec): " << t_msec << "\n";
  std::cout << "Speed =            "     << nInst/t_msec          << " KIPS\n";
  std::cout << "uops/inst =        "     << ((double)nUops)/nInst << "\n";
  std::cout << "#Hits on Crack cache = " << nHit                  << "\n";

}

void Scexec::fetch_crack() {

  uint32_t pc = crackInst->getPC();

  TranslationType ttype = core.getTType(); //FIXME: Unify type in core and crackInst
#ifdef ENABLE_CRACK_CACHE 
  uint32_t hash_idx = hash(pc&0XFFFFFFFE) & (CRACK_CACHE_SIZE-1);
  if (!crackCache[hash_idx].isEqual(pc) || !crackCache[hash_idx].isITBlock()){
#endif
    int32_t insn = prog->get_code32(pc&0xFFFFFFFE);

#ifdef SPARC32
    if(ttype == SPARC32) { //SPARC is big-endian so need to interchange byte order
      insn = ((insn & 0xFF) << 24) | 
             (((insn>>8) & 0xFF) << 16) | 
             (((insn>>16) & 0xFF) << 8) | 
             ((insn>>24) & 0xFF);
    }
#endif

    rinst->set(insn, pc);
    if (ttype == ARM){
      crackInstARM->setPC(crackInst->getPC());
      crackInst = crackInstARM;
#ifdef SPARC32
    }else if(ttype == SPARC32){
      crackInstSPARC->setPC(crackInst->getPC());
      crackInst = crackInstSPARC;
#endif
    }else{
      crackInstThumb->setPC(crackInst->getPC());
      crackInst = crackInstThumb;
    }

    crackInst->settransType(ttype);
    core.setCrackInt(crackInst);

    crackInst->expand(rinst);
#ifdef ENABLE_CRACK_CACHE      
#error "Do not run with ENABLE_CRACK_CACHE for now!"
    crackCache[hash_idx] = *rinst;
    crackCacheTType[hash_idx] = crackInst->gettransType();
#ifdef DEBUG_LIGHT      
    crackTrace[hash_idx] ++;
#endif
  }else{
    rinst = &crackCache[hash_idx];

    if (crackCacheTType[hash_idx] == THUMB32 || crackCacheTType[hash_idx] == THUMB) {
      crackInstThumb->setPC(crackInst->getPC());
      crackInst = crackInstThumb;
    }else if (crackCacheTType[hash_idx] == ARM){
      crackInstARM->setPC(crackInst->getPC());
      crackInst = crackInstARM;
#ifdef SPARC32
    }else if (crackCacheTType[hash_idx] == SPARC32){
      crackInstSPARC->setPC(crackInst->getPC());
      crackInst = crackInstSPARC;
#endif
    }else{
      I(0);
    }

    crackInst->settransType(crackCacheTType[hash_idx]);
    nHit ++;
#ifdef DEBUG2
    printf("insn from cache:0x%x\n", rinst->getInsn() );
#endif
    I(rinst->getPC() == pc);
  }
#endif

#ifdef DEBUG2
  printf("expanded:\n");
  for(size_t j=0;j<rinst->getNumInst();j++) {
    const UOPPredecType *inst = rinst->getInstRef(j);
    inst->dump("+");
    printf("\n");
  }
#endif
}

void Scexec::execute(){
#ifdef DEBUG2
  printf("executed:\n");
#endif

  core.resetNOPStatus();
  for(size_t j=0;j<rinst->getNumInst();j++) {
    nUops++;
    const UOPPredecType *inst = rinst->getInstRef(j);
    //------------------------

    //printf("reset NOPStatus \n");

    core.execute(inst);

#ifdef DEBUG2
    // core.mod_reset_accumulate(1, NULL);
    core.dump("-");
    core.sync();
#endif
    if (core.getFlushDecode()){
      break;
    }
  }
#ifdef DEBUG2
  printf ("inITBlock %d, NOPStatus %d, flushDecode %d \n", rinst->isInITBlock(), core.getNOPStatus(), core.getFlushDecode() );
#endif

  if ( !rinst->isInITBlock() || (rinst->isInITBlock() && (!core.getNOPStatus()) && !core.getFlushDecode())  ) {
    validate();
  } else {
    printf ("no validation flag set \n");
  }

  // now, reset the accumulated mod array.
  // core.mod_reset_accumulate(0, NULL);

    if (core.getFlushDecode()){
      core.resetFlushDecode();
    } else {
      crackInst->advPC();
    }
#ifdef DEBUG2
    crackInst->dumpPC(fid);
#endif
}

void Scexec::validate() {
  char exp_pc_str[32];
  char exp_inst_str[32];

  char tmp_str[32];

  char emul_pc_str[32];
  char emul_inst_str[32];

  char inst_num[32];
  char vInst_str[32];

  regex_t re1, re2, re3;
  regmatch_t match[4];
  int ret, ret1, ret2, i, j;
  char reg_num[3];
  char reg_val[32];
  char f_reg_val[32];
  int reg_num_val;

  int exp_total_num_reg_mod = 0;
  long file_offset;

  if (fp == NULL) {
#ifdef DEBUG2
    printf("No ISA validation. Return \n");
#endif
    return;
  } 

  // Check to see if the instruction falls in ldrex/strex block
  validate_insn = true;
  if(ldrex_block) {
    if (isDMBsyInsn(rinst->getInsn() ) ) {
      ldrex_block   = false;
      validate_insn = true;
    } else {
      printf("inst 0x%08x pc 0x%08x falls in ldrex/strex block. Do not validate \n", rinst->getInsn(), rinst->getPC() );
      validate_insn = false;
    }
  } else if(isLDREXinsn(rinst->getInsn())) {
    ldrex_block   = true;
    validate_insn = false;
  }
  if (!validate_insn) {
    // printf ("do not validate instruction, it falls in ldrex/strex block \n");
    return;
  }

  memset(inst_num, '\0', 32);
  if (!feof(fp)) {
    fgets(inst_num, sizeof inst_num, fp);
  } else {
    printf ("trace file EOF. Do not validate \n");
    return;
  }
  inst_num[strlen (inst_num) - 1] = '\0';
  
  memset(tmp_str, '\0', 32);
  //From app: PC
  file_offset = ftell(fp); // remember in case we need to rollback
  sprintf(tmp_str, "0x%08x", rinst->getPC());
  strcpy(emul_pc_str, "pc: ");
  strcat(emul_pc_str, tmp_str);

  //From log generated on ARM machine. PC 
  fgets(exp_pc_str, sizeof exp_pc_str, fp);
  exp_pc_str[strlen (exp_pc_str) - 1] = '\0';

  if (strcmp(exp_pc_str, emul_pc_str) != 0) {
    if(core.getNOPStatus()) {
      // don't validate the instruction, this is a conditional move
      // and trace file won't have this instruction
      
      // Set the trace file pointer back to the beginning of PC line
      fseek(fp, file_offset, SEEK_SET);
      printf ("conditional move, do not validate the instruction \n");
      return;
    }
  }

  vInst++ ;

  // This check is to validate a range of instructions.
  // Match the instruction number from trace file with emulator 
  // instruction number to see if we need to validate.
  sprintf(vInst_str, "Instruction number: %d", vInst);
  printf ("inst num: emul %s, trace file %s \n", vInst_str, inst_num );
  if (strcmp(vInst_str, inst_num)) {
    printf ("   do not match so don't validate \n");
    exit(0);
    // roll back to the beginning of the trace file
//    fseek(fp, 0, SEEK_SET);
//    return;
   }

  // OK. We are ready to validate

  // 1. Validate PC

  if (strcmp(exp_pc_str, emul_pc_str) == 0) {
    // printf("    PASS: pc value matches \n");
  } else {
    printf("    FAIL: pc values don't match. nInst %d  \n", nInst);
    printf ("     Expected: %s Emulator: %s \n", exp_pc_str, emul_pc_str );
    I(0);
    exit(0);
  }

  // 2. Validate instruction

  //From app: Instruction
  memset(tmp_str, '\0', 32);
  sprintf(tmp_str, "0x%08x", rinst->getInsn());
  strcpy(emul_inst_str, "inst: ");
  strcat(emul_inst_str, tmp_str);

  //From log generated on ARM machine. Instruction
  fgets(exp_inst_str, sizeof exp_inst_str, fp);
  exp_inst_str[strlen (exp_inst_str) - 1] = '\0';

  if (strcmp(exp_inst_str, emul_inst_str) == 0) {
    // printf("    PASS: inst value matches \n");
  } else {
    printf("    FAIL: inst values don't match. nInst %d \n", nInst);
    printf ("     Expected: %s Emulator: %s \n", exp_inst_str, emul_inst_str);
    I(0);
    exit(0);
  }

  // 3. Validate registers

  ret = regcomp(&re1, "-+", REG_EXTENDED);
  ret = regcomp(&re2, "^r([0-9]+)=0x(.*)", REG_EXTENDED);
  ret = regcomp(&re3, "^s([0-9]+)=0x(.*)", REG_EXTENDED);

  while (fgets(tmp_str, sizeof tmp_str, fp) != NULL) {
    tmp_str[strlen (tmp_str) - 1] = '\0';
                 
    ret = regexec(&re1, tmp_str, 0, NULL, 0);
    if(!ret) {
      // printf("------End of Section-------\n\n");
      if(firstIteration == true) {
        firstIteration = false;
      }
      exp_total_num_reg_mod = 0;

      break;
    } else {
      // printf("%s \n", tmp_str);
      ret1 = regexec(&re2, tmp_str, 3, match, 0);
      ret2 = regexec(&re3, tmp_str, 3, match, 0);

      if(!ret1 || !ret2) {
        j = 0;
        for(i = match[1].rm_so; i < match[1].rm_eo; i++) {
          reg_num[j] = tmp_str[i];
          j++;
        }
        reg_num[j] = '\0';

        j = 0;
        for(i = match[2].rm_so; i < match[2].rm_eo; i++) {
          reg_val[j] = tmp_str[i];
          j++;
        }
        reg_val[j] = '\0';
        printf("reg_val %s \n", reg_val);
        sprintf(f_reg_val, "0x%08x", strtol(reg_val,NULL,16));

        printf("reg_num %s reg_val %s f_reg_val %s \n", reg_num, reg_val, f_reg_val);

        // Convert reg_num string to integer value.
        reg_num_val = atoi(reg_num);

        // Ignore 1st iteration for now since the initialized values are different from APP and ARM
        if (firstIteration == false) {
          core.validateRegister(reg_num_val, f_reg_val);
        }

        exp_total_num_reg_mod++;
        
      } else {
        printf("This should never happen!!! Did not find register info, Check the ARM Log file\n");
        I(0);
      }
    } 
    memset(tmp_str, '\0', 32);
  } 
}

bool Scexec::isDMBsyInsn(RAWInstType insn) {
  char tmp_str[32];
  memset(tmp_str, '\0', 32);
  sprintf(tmp_str, "0x%08x", rinst->getInsn());
  if (!(strcmp(tmp_str, "0x8f5ff3bf")) || !(strcmp(tmp_str, "0xf57ff05f")) ) {
    return true;
  } else {
    return false;
  }
}

bool Scexec::isLDREXinsn(RAWInstType insn) {

  uint32_t o1 = (insn << 20);
  uint32_t o2 = ((insn << 4) >> 24);

  if((insn & 0x0f00e850) == 0x0f00e850) {
    //Thumb
    return true;
  } else if( (o1 == 0xF9F00000) & (o2 == 0x00000019) ) {
    //ARM
    printf("inst: 0x%08x is ldrex. o1 0x%08x o2 0x%08x \n", insn, o1, o2);
    return true;
  }
  return false;
}
