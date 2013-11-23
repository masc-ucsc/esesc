#!/usr/bin/python2

import sys
import re
from MacroLibrary import *
from MacroParser import *
import random
from random import choice

def generatePrologue(f):
  f.write('.syntax unified \n')
  f.write('.global main \n')
  f.write('main: \n')
  f.write('push {ip, lr} \n')
  for i in xrange(1, 13):
    f.write('MOV R%s, #%s \n' % (i, i) ) 
  f.write('\n')

def generateRandomInstructions(lib, f):
  d = {}
  macro_num = 0

  #inst_class = ["ARM_data_processing_register", "ARM_data_processing_register_shifted_register",
  #            "ARM_data_processing_immediate", "ARM_multiply_and_multiplyaccumulate",
  #            "ARM_saturating_addition_and_subtraction", "ARM_halfword_multiply_and_multiplyaccumulate",
  #            "ARM_extra_load_store", "ARM_extra_load_store_unprivileged", "ARM_sync_primitives",
  #            "ARM_MSR_and_hints", "ARM_misc", "ARM_load_store_word_unsigned_byte",
  #            "ARM_branch_branch_with_link_block_data_transfer"]

  # Everything except 'ARM_sync_primitives"
  inst_class = ["ARM_data_processing_register", "ARM_data_processing_register_shifted_register",
              "ARM_data_processing_immediate", "ARM_multiply_and_multiplyaccumulate",
              "ARM_saturating_addition_and_subtraction", "ARM_halfword_multiply_and_multiplyaccumulate",
              "ARM_extra_load_store", "ARM_extra_load_store_unprivileged",
              "ARM_MSR_and_hints", "ARM_misc", "ARM_load_store_word_unsigned_byte",
              "ARM_branch_branch_with_link_block_data_transfer"]

  total_num_inst = 0
  random.seed()

  # XXX Testing
  while total_num_inst <= 1000:
    # 1. Choose instruction class at random
    size = len(inst_class) - 1
    n = random.randint(0,size)
    print 'class', n, inst_class[n]

    # 2. Get an instruction macro of the given type from the 'Instruction Macro Library'
    macro = lib.getMacroForType(inst_class[n])
    # print 'macro:', macro
    macro_num += 1
    key = 'f' + str(macro_num)

    d[key] = []


    # 3. Generate instructions from the instruction macro.
    total_num_inst += parseAndGenerateInst(macro, inst_class[n], d[key])
    # print 'total_num_inst', total_num_inst

  # now create a test assembly program
  for key in d.keys():
    # print 'key: ', key
    # XXX Add support for BLX
    branch = choice(['B', 'BL'])
    if branch == 'B':
      f.write('mov lr, pc \n')
    f.write(branch + ' ' + key + '\n')
  f.write('\n')
  f.write('mov r0, #0 \n')
  f.write('pop {ip, pc} \n \n')

  # Now, main done. Create sub-routines. 
  end_of_proc = 0
  for key in d.keys():
    if end_of_proc:
      f.write('\n')
      f.write('  mov r0, #0 \n')
      f.write('  mov pc, lr \n\n')
    f.write(key + ': \n')
    for val in d.get(key):
      if val in ['ADR']:
        f.write('  ADR r0, ' + key + '\n')   
      elif re.search('label', val):
        #print 'writing: ' + '  ' +  val[:-5] + key 
        f.write('  ' +  val[:-5] + key + '\n')
      else:
        f.write('  ' + val + '\n')
      end_of_proc = 1

  f.write('\n')
  f.write('  mov r0, #0 \n')
  f.write('  mov pc, lr \n\n')

def generateEpilogue(f):
  f.write('.data \n')
  f.write('myarr: \n')
  for i in range(0, 257):
    num_str = '0x' + str("%x" % random.randint(0, 0xffffffff))
    f.write('  .word  ' + num_str )
    f.write('\n')
  f.write('.bss \n')
  f.write('out: \n')
  f.write('  .space 256 \n')

def main():

  # preamble
  # generate instructions
  lib = MacroLibrary()

  if (len(sys.argv) < 2):
    print('\n')
    print('Usage:   generateRIT.py out_RIT_file')
    print('\n')
    sys.exit(1)
  f = open(sys.argv[1], 'w')
  generatePrologue(f)
  generateRandomInstructions(lib, f)
  generateEpilogue(f)

if __name__ == "__main__":
  main()
