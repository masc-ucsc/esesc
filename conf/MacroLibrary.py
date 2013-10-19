#!/usr/bin/python

import random

class MacroLibrary(object):

  def __init__(self):

    self.ARM_data_processing_register = ["AND{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "EOR{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "SUB{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "RSB{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "ADD{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "ADC{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "SBC{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "RSC{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "TST<c> <Rn>,<Rm>{,<shift>}",
    "TEQ<c> <Rn>,<Rm>{,<shift>}",
    "CMP<c> <Rn>,<Rm>{,<shift>}",
    "CMN<c> <Rn>,<Rm>{,<shift>}",
    "ORR{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "MOV{S}<c> <Rd>,<Rm>",
    "LSL{S}<c> <Rd>,<Rm>,#<imm5>",
    "LSR{S}<c> <Rd>,<Rm>,#<imm5>",
    "ASR{S}<c> <Rd>,<Rm>,#<imm5>",
    "RRX{S}<c> <Rd>,<Rm>",
    "ROR{S}<c> <Rd>,<Rm>,#<imm5>",
    "BIC{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}",
    "MVN{S}<c> <Rd>,<Rm>{,<shift>}"]

    self.ARM_data_processing_register_shifted_register = ["AND{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "EOR{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "SUB{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "RSB{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "ADD{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "ADC{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "SBC{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "RSC{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "TST<c> <Rn>,<Rm>,<type> <Rs>",
    "TEQ<c> <Rn>,<Rm>,<type> <Rs>",
    "CMP<c> <Rn>,<Rm>,<type> <Rs>",
    "CMN<c> <Rn>,<Rm>,<type> <Rs>",
    "ORR{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "LSL{S}<c> <Rd>,<Rn>,<Rm>",
    "LSR{S}<c> <Rd>,<Rn>,<Rm>",
    "ASR{S}<c> <Rd>,<Rn>,<Rm>",
    "ROR{S}<c> <Rd>,<Rn>,<Rm>",
    "BIC{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>",
    "MVN{S}<c> <Rd>,<Rm>,<type> <Rs>"]

    self.ARM_data_processing_immediate = ["AND{S}<c> <Rd>,<Rn>,#<const>",
    "EOR{S}<c> <Rd>,<Rn>,#<const>",
    "SUB{S}<c> <Rd>,<Rn>,#<const>",
    "ADR<c> <Rd>,<label>",
    "RSB{S}<c> <Rd>,<Rn>,#<const>",
    "ADD{S}<c> <Rd>,<Rn>,#<const>",
    "ADC{S}<c> <Rd>,<Rn>,#<const>",
    "SBC{S}<c> <Rd>,<Rn>,#<const>",
    "RSC{S}<c> <Rd>,<Rn>,#<const>",
    "TST<c> <Rn>,#<const>",
    "TEQ<c> <Rn>,#<const>",
    "CMP<c> <Rn>,#<const>",
    "CMN<c> <Rn>,#<const>",
    "ORR{S}<c> <Rd>,<Rn>,#<const>",
    "MOV{S}<c> <Rd>,#<const>",
    "MOVW<c> <Rd>,#<imm16>",
    "BIC{S}<c> <Rd>,<Rn>,#<const>",
    "MVN{S}<c> <Rd>,#<const>"]

    self.ARM_multiply_and_multiplyaccumulate = ["MUL{S}<c> <Rd>,<Rn>,<Rm>",
    "MLA{S}<c> <Rd>,<Rn>,<Rm>,<Ra>",
    "UMAAL<c> <RdLo>,<RdHi>,<Rn>,<Rm>",
    "MLS<c> <Rd>,<Rn>,<Rm>,<Ra>",
    "UMULL{S}<c> <RdLo>,<RdHi>,<Rn>,<Rm>",
    "UMLAL{S}<c> <RdLo>,<RdHi>,<Rn>,<Rm>",
    "SMULL{S}<c> <RdLo>,<RdHi>,<Rn>,<Rm>",
    "SMLAL{S}<c> <RdLo>,<RdHi>,<Rn>,<Rm>"]

    self.ARM_saturating_addition_and_subtraction = ["QADD<c> <Rd>,<Rm>,<Rn>",
    "QSUB<c> <Rd>,<Rm>,<Rn>",
    "QDADD<c> <Rd>,<Rm>,<Rn>",
    "QDSUB<c> <Rd>,<Rm>,<Rn>"]

    self.ARM_halfword_multiply_and_multiplyaccumulate = ["SMLA<x><y><c> <Rd>,<Rn>,<Rm>,<Ra>",
    "SMLAW<y><c> <Rd>,<Rn>,<Rm>,<Ra>",
    "SMULW<y><c> <Rd>,<Rn>,<Rm>",
    "SMLAL<x><y><c> <RdLo>,<RdHi>,<Rn>,<Rm>",
    "SMUL<x><y><c> <Rd>,<Rn>,<Rm>"]

    self.ARM_extra_load_store = ["STRH<c> <Rt>,[<Rn>,+/-<Rm>]{!}",
    "STRH<c> <Rt>,[<Rn>,+/-<Rm>]",
    "LDRH<c> <Rt>,[<Rn>,+/-<Rm>]{!}",
    "LDRH<c> <Rt>,[<Rn>,+/-<Rm>]",
    "STRH<c> <Rt>,[<Rn>{,#+/-<imm8>}]",
    "STRH<c> <Rt>,[<Rn>],#+/-<imm8>",
    "STRH<c> <Rt>,[<Rn>,#+/-<imm8>]!",
    "LDRH<c> <Rt>,[<Rn>{,#+/-<imm8>}]",
    "LDRH<c> <Rt>,[<Rn>],#+/-<imm8>",
    "LDRH<c> <Rt>,[<Rn>,#+/-<imm8>]!",
    "LDRH<c> <Rt>,<label>",
    "LDRH<c> <Rt>,[PC,#+/-<imm8>]",
    "LDRD<c> <Rt>,<Rt2>,[<Rn>,+/-<Rm>]{!}",
    "LDRD<c> <Rt>,<Rt2>,[<Rn>],+/-<Rm>",
    "LDRSB<c> <Rt>,[<Rn>,+/-<Rm>]{!}",
    "LDRSB<c> <Rt>,[<Rn>],+/-<Rm>",
    "LDRD<c> <Rt>,<Rt2>,[<Rn>{,#+/-<imm8>}]",
    "LDRD<c> <Rt>,<Rt2>,[<Rn>],#+/-<imm8>",
    "LDRD<c> <Rt>,<Rt2>,[<Rn>,#+/-<imm8>]!",
    "LDRSB<c> <Rt>,[<Rn>{,#+/-<imm8>}]",
    "LDRSB<c> <Rt>,[<Rn>],#+/-<imm8>",
    "LDRSB<c> <Rt>,[<Rn>,#+/-<imm8>]!",
    "LDRSB<c> <Rt>,<label>",
    "LDRSB<c> <Rt>,[PC,#+/-<imm8>]", 
    "STRD<c> <Rt>,<Rt2>,[<Rn>,+/-<Rm>]{!}",
    "STRD<c> <Rt>,<Rt2>,[<Rn>],+/-<Rm>",
    "LDRSH<c> <Rt>,[<Rn>,+/-<Rm>]{!}",
    "LDRSH<c> <Rt>,[<Rn>],+/-<Rm>",
    "STRD<c> <Rt>,<Rt2>,[<Rn>{,#+/-<imm8>}]",
    "STRD<c> <Rt>,<Rt2>,[<Rn>],#+/-<imm8>",
    "STRD<c> <Rt>,<Rt2>,[<Rn>,#+/-<imm8>]!",
    "LDRSH<c> <Rt>,[<Rn>{,#+/-<imm8>}]",
    "LDRSH<c> <Rt>,[<Rn>],#+/-<imm8>",
    "LDRSH<c> <Rt>,[<Rn>,#+/-<imm8>]!",
    "LDRSH<c> <Rt>,<label>",
    "LDRSH<c> <Rt>,[PC,#+/-<imm8>]"]

    self.ARM_extra_load_store_unprivileged = ["STRHT<c> <Rt>,[<Rn>],+/-<Rm>",
    "LDRHT<c> <Rt>,[<Rn>]{,#+/-<imm8>}",
    "LDRHT<c> <Rt>,[<Rn>],+/-<Rm>",
    "LDRSBT<c> <Rt>,[<Rn>]{,#+/-<imm8>}",
    "LDRSBT<c> <Rt>,[<Rn>],+/-<Rm>",
    "LDRSHT<c> <Rt>,[<Rn>]{,#+/-<imm8>}",
    "LDRSHT<c> <Rt>,[<Rn>],+/-<Rm>"]

    # Not yet included in (RIT) Random Instruction Tests
    self.ARM_sync_primitives = ["SWP{B}<c> <Rt>,<Rt2>,[<Rn>]",
    "STREX<c> <Rd>,<Rt>,[<Rn>]",
    "LDREX<c> <Rt>,[<Rn>]",
    "STREXD<c> <Rd>,<Rt>,<Rt2>,[<Rn>]",
    "LDREXD<c> <Rt>,<Rt2>,[<Rn>]",
    "STREXB<c> <Rd>,<Rt>,[<Rn>]",
    "LDREXB<c> <Rt>, [<Rn>]",
    "STREXH<c> <Rd>,<Rt>,[<Rn>]",
    "LDREXH<c> <Rt>, [<Rn>]"]

    self.ARM_MSR_and_hints = ["NOP<c>", "YIELD<c>", "WFE<c>", "WFI<c>", "SEV<c>", "DBG<c> #<option>", 
    "MSR<c> <spec_reg>,#<const>"]

    self.ARM_misc = ["MRS<c> <Rd>,<spec_reg>", "CLZ<c> <Rd>,<Rm>"]

    self.ARM_load_store_word_unsigned_byte = ["STR<c> <Rt>,[<Rn>{,#+/-<imm12>}]",
    "STR<c> <Rt>,[<Rn>],#+/-<imm12>",
    "STR<c> <Rt>,[<Rn>,#+/-<imm12>]{!}",
    "STR<c> <Rt>,[<Rn>,+/-<Rm>{, <shift>}]{!}",
    "STR<c> <Rt>,[<Rn>],+/-<Rm>{, <shift>}",
    "STRT<c> <Rt>,[<Rn>]{,+/-<imm12>}",
    "STRT<c> <Rt>,[<Rn>],+/-<Rm>{, <shift>}",
    "LDR<c> <Rt>,[<Rn>{,#+/-<imm12>}]",
    "LDR<c> <Rt>,[<Rn>],#+/-<imm12>",
    "LDR<c> <Rt>,[<Rn>,#+/-<imm12>]{!}",
    "LDR<c> <Rt>,<label>",
    "LDR<c> <Rt>,[PC,#+/-<imm12>]",
    "LDR<c> <Rt>,[<Rn>,+/-<Rm>{, <shift>}]{!}",
    "LDR<c> <Rt>,[<Rn>],+/-<Rm>{, <shift>}",
    "LDRT<c> <Rt>,[<Rn>]{,#+/-<imm12>}",
    "LDRT<c> <Rt>,[<Rn>],+/-<Rm>{, <shift>}",
    "STRB<c> <Rt>,[<Rn>{,#+/-<imm12>}]",
    "STRB<c> <Rt>,[<Rn>],#+/-<imm12>",
    "STRB<c> <Rt>,[<Rn>,#+/-<imm12>]{!}",
    "STRB<c> <Rt>,[<Rn>,+/-<Rm>{, <shift>}]{!}",
    "STRB<c> <Rt>,[<Rn>],+/-<Rm>{, <shift>}",
    "STRBT<c> <Rt>,[<Rn>],#+/-<imm12>",
    "STRBT<c> <Rt>,[<Rn>],+/-<Rm>{, <shift>}",
    "LDRB<c> <Rt>,[<Rn>{,#+/-<imm12>}]",
    "LDRB<c> <Rt>,[<Rn>],#+/-<imm12>",
    "LDRB<c> <Rt>,[<Rn>,#+/-<imm12>]{!}",
    "LDRB<c> <Rt>,<label>",
    "LDRB<c> <Rt>,[PC,#+/-<imm12>]",
    "LDRB<c> <Rt>,[<Rn>,+/-<Rm>{, <shift>}]{!}",
    "LDRB<c> <Rt>,[<Rn>],+/-<Rm>{, <shift>}",
    "LDRBT<c> <Rt>,[<Rn>],#+/-<imm12>",
    "LDRBT<c> <Rt>,[<Rn>],+/-<Rm>{, <shift>}"]

    self.ARM_branch_branch_with_link_block_data_transfer = ["STMDA<c> <Rn>{!},<registers>",
    "LDMDA<c> <Rn>{!},<registers>",
    "STM<c> <Rn>{!},<registers>",
    "LDMDB<c> <Rn>{!},<registers>",
    "STMIB<c> <Rn>{!},<registers>",
    "LDMIB<c> <Rn>{!},<registers>",
    "STMDA<c> <Rn>,<registers>",
    "STMDB<c> <Rn>,<registers>",
    "STMIA<c> <Rn>,<registers>",
    "STMIB<c> <Rn>,<registers>",
    "LDMDA<c> <Rn>,<registers_without_pc>",
    "LDMDB<c> <Rn>,<registers_without_pc>",
    "LDMIA<c> <Rn>,<registers_without_pc>",
    "LDMIB<c> <Rn>,<registers_without_pc>"
    # B, BL are covered
    #"B<c> <label>",
    #"BL<c> <label>", 
    #"BLX <label>"
    ]

  def getMacroForType(self,type):

    random.seed()

    if type == 'ARM_data_processing_register':
      name = self.ARM_data_processing_register
    if type == 'ARM_data_processing_register_shifted_register':
      name = self.ARM_data_processing_register_shifted_register
    if type == 'ARM_data_processing_immediate':
      name = self.ARM_data_processing_immediate
    if type == 'ARM_multiply_and_multiplyaccumulate':
      name = self.ARM_multiply_and_multiplyaccumulate
    if type == 'ARM_saturating_addition_and_subtraction':
      name = self.ARM_saturating_addition_and_subtraction
    if type == 'ARM_halfword_multiply_and_multiplyaccumulate':
      name = self.ARM_halfword_multiply_and_multiplyaccumulate
    if type == 'ARM_MSR_and_hints':
      name = self.ARM_MSR_and_hints
    if type == 'ARM_misc':
      name = self.ARM_misc
    if type == 'ARM_extra_load_store':
      name = self.ARM_extra_load_store
    if type == 'ARM_extra_load_store_unprivileged':
      name = self.ARM_extra_load_store_unprivileged
    if type == 'ARM_load_store_word_unsigned_byte':
      name = self.ARM_load_store_word_unsigned_byte
    if type == 'ARM_branch_branch_with_link_block_data_transfer':
      name = self.ARM_branch_branch_with_link_block_data_transfer

    s = len(name) - 1
    n = random.randint(0,s)
    return name[n]
