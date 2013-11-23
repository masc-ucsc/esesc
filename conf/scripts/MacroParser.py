#!/usr/bin/python

import sys
import re
import random
from random import choice
random.seed()

def ROR(x, n):
    mask = (2L**n) - 1
    mask_bits = x & mask
    return (x >> n) | (mask_bits << (32 - n))

def getXY():

  return choice(['B', 'T'])

def getReg(reg, start=0, end=12):

  # XXX
  if reg:
    return 'R' + str(random.randint(start, end))

  return ' '

def getConst():

  # 4 bit rotate, 8 bit immediate
  return str( (random.randint(1, 255) << random.randrange(0, 16, 2)  ))

def getShift(shift):

  shift_type = ['LSL', 'LSR', 'ASR', 'ROR', 'RRX', '']

  if shift:
    s = choice(shift_type)
    if s == 'LSL':
      s += '#' + str(random.randint(1, 31))
    if s == 'LSR':
      s += '#' + str(random.randint(1, 32))
    if s == 'ASR':
      s += '#' + str(random.randint(1, 32))
    if s == 'ROR':
      s += '#' + str(random.randint(1, 31))

  return s

def getType(type):
  
  type = ['LSL', 'LSR', 'ASR', 'ROR', '']
  if type:
    s = choice(type)
    
  return s

def getImm(imm):
  # print 'imm ', imm

  if imm == '#<imm5>':
    return str(random.randint(0, 31))
  elif imm == '#<imm16>':
    return str(random.randint(0, 65535))
  elif imm == '#<imm8>':
    return str(random.randint(0, 255))
  elif imm == '#<imm12>':
    return str(random.randint(0, 4095))

  return ' '

def addSetCondCodesInst(inst, list, num_inst):
  match = re.match('(\S+)\s+(.*)', inst)
  cmd = match.group(1)
  rest = match.group(2)
 
  list.append(cmd + 'S' + ' ' + rest)
  num_inst += 1

  return num_inst


def addCondInst(inst, list, num_inst):
  match = re.match('(\S+)\s+(.*)', inst)
  cmd = match.group(1)
  rest = match.group(2)

  if match:
    # z = 1 
    list.append('msr apsr_nzcvq, #0x40000000')
    list.append(cmd + 'eq' + ' ' + rest) 
    list.append(cmd + 'ne' + ' ' + rest)

    # z = 1, c = 0
    list.append(cmd + 'hi' + ' ' + rest)
    list.append(cmd + 'ls' + ' ' + rest)

    # n == v (0 in this case)
    list.append(cmd + 'ge' + ' ' + rest)
    list.append(cmd + 'lt' + ' ' + rest)

    # c = 1
    list.append('msr apsr_nzcvq, #0x20000000')
    list.append(cmd + 'cs' + ' ' + rest)
    list.append(cmd + 'cc' + ' ' + rest)

    # c = 1, z = 0
    list.append(cmd + 'hi' + ' ' + rest)
    list.append(cmd + 'ls' + ' ' + rest)

    # n = 1
    list.append('msr apsr_nzcvq, #0x80000000')
    list.append(cmd + 'mi' + ' ' + rest)
    list.append(cmd + 'pl' + ' ' + rest)

    # v = 1
    list.append('msr apsr_nzcvq, #0x10000000')
    list.append(cmd + 'vs' + ' ' + rest)
    list.append(cmd + 'vc' + ' ' + rest)

    # n = 0, v = 1 (n != v)
    list.append(cmd + 'ge' + ' ' + rest)
    list.append(cmd + 'lt' + ' ' + rest)

    list.append('msr apsr_nzcvq, #0x00000000')
    list.append(cmd + 'eq' + ' ' + rest)
    list.append(cmd + 'ne' + ' ' + rest)
    list.append(cmd + 'cs' + ' ' + rest)
    list.append(cmd + 'cc' + ' ' + rest)
    list.append(cmd + 'mi' + ' ' + rest)
    list.append(cmd + 'pl' + ' ' + rest)
    list.append(cmd + 'vs' + ' ' + rest)
    list.append(cmd + 'vc' + ' ' + rest)

    # z = 0, n = v = 1
    list.append('msr apsr_nzcvq, #0x90000000')
    list.append(cmd + 'gt' + ' ' + rest)
    list.append(cmd + 'le' + ' ' + rest)

    # z = 1, n = 1, v = 0
    list.append('msr apsr_nzcvq, #0x90000000')
    list.append(cmd + 'gt' + ' ' + rest)
    list.append(cmd + 'le' + ' ' + rest)

    num_inst += 35

  return num_inst

def handle_ARM_branch_branch_with_link_block_data_transfer(macro, list):
  inst = ''
  num_inst = 0

  list1 = []
  list2 = []

  Rt_reg = ''
  Rm_reg = ''
  Rn_reg = ''

  match = re.search('([A-Z]{1,6})(<c>)?\s+(<Rn>)(\{!\})?,(<.*>)', macro)

  if match:
    cmd = match.group(1)
    cc = match.group(2)
    Rn = match.group(3)
    pre_index = match.group(4)
    reglist = match.group(5)

  if Rn:
    Rn_reg = getReg('Rn',0,5)

  if cmd[0] == 'L': # loads
    list2.append('LDR ' + Rn_reg + ',' + '=' + choice(['out', 'myarr'])) 
  elif cmd[0] == 'S': # stores
    list2.append('LDR ' + Rn_reg + ',' + '=out') 

  inst += cmd + ' ' + Rn_reg

  if pre_index:
    inst += '!'

  num_regs = random.randint(1, 6) + 6

  inst += ',' + '{' + 'R6' + '-' + 'R' + str(num_regs) + '}'

  # print 'inst ', inst
  # print '\n'

  list1.append(inst)
  num_inst += 1

  if cc:
    num_inst += addCondInst(inst, list1, num_inst)

  for e in list1:
    for el in list2:
      list.append(el)
      num_inst += 1
    list.append(e)
    num_inst += 1

  return num_inst
  

def handle_ARM_load_store_word_and_unsigned_byte(macro, list):
  inst = ''
  num_inst = 0

  list1 = []
  list2 = []

  Rt_reg = ''
  Rm_reg = ''
  Rn_reg = ''

  match = re.search('([A-Z]{1,6})(<c>)?\s+(<Rt>),(<label>)', macro)

  match1 = re.search('([A-Z]{1,6})(<c>)?\s+(<Rt>),\[(<Rn>|PC)(\])?(\{)?,(#)?\+/-(<Rm>|<imm8>|<imm12>)(\})?(\{,\s+<shift>\})?(\])?(\{!\})?', macro)

  label = ''

  if match:
    cmd = match.group(1)
    cc = match.group(2)
    Rt = match.group(3)
    label = match.group(4)

  elif match1:
    cmd = match1.group(1)
    cc = match1.group(2)
    Rt = match1.group(3)
    Rn_or_pc = match1.group(4)
    square_0 = match1.group(5)
    curly_0 = match1.group(6)
    hash = match1.group(7)
    Rm_or_imm = match1.group(8)
    curly_1 = match1.group(9)
    shift = match1.group(10)
    square_1 = match1.group(11)
    pre_index = match1.group(12)  

    # print 'cmd', cmd, 'cc', cc, 'Rt', Rt, 'Rn_or_pc', Rn_or_pc
    # print 'square_0', square_0, 'curly_0', curly_0, 'hash', hash, 'Rm_or_imm', Rm_or_imm
    # print 'curly_1', curly_1, 'shift', shift, 'square_1', square_1, 'pre_index', pre_index
    # print '\n'

  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd + ' '

  if Rt:
    Rt_reg = getReg('Rt')
  inst += Rt_reg

  if label:
    inst += ', ' + 'label'
  else:
    inst += ',' + '['

    if re.search('Rn', Rn_or_pc):
      while 1:
        Rn_reg = getReg('Rn')
        if Rn_reg != Rt_reg:
          break
      list2.append('LDR ' + Rn_reg + ',' + '=' + choice(['out', 'myarr']))
    elif re.search('PC', Rn_or_pc):
      Rn_reg = 'PC'

    inst += Rn_reg

    if square_0:
      inst += ']' 
    #if curly_0:
    #  inst += '{' 

    inst += ','

    if hash:
      inst += '#'

    sign = choice(['+', '-'])
    inst += sign
    if sign == '-':
      if Rn_reg:
        if Rn_reg != 'PC':
          list2.append('ADD ' + Rn_reg + ',' + '#100') 

    if Rm_or_imm:
      if re.search('Rm', Rm_or_imm):
        reg_offset_val = random.randint(0, 100)
        while(1):
          Rm_reg = getReg('Rm')
          if Rm_reg != Rn_reg and Rm_reg != Rt_reg:
            break
        list.append('MOV ' + str(Rm_reg) + ',' + str(reg_offset_val))
        inst += Rm_reg
      elif re.search('imm12', Rm_or_imm):
        # XXX use imm8 for now
        inst += getImm('#<imm8>')

    if shift:
      sh = getShift(shift)
      # print 'sh: ', sh 

      shift_by = 0
      if sh:
        match = re.search('([A-Z]{1,8})#?(.*)?', sh)

        if match:
          shift_type = match.group(1)

        # hard-coding to avoid seg faults.
        reg_offset_val = 4
        shift_by = 1
        list2.append('MOV ' + str(Rm_reg) + ',' + str(reg_offset_val) )

        if shift_type == 'LSL':
          shift_val = reg_offset_val << shift_by
        elif shift_type == 'LSR':
          shift_val = reg_offset_val >> shift_by
        elif shift_type == 'ASR':
          shift_val = reg_offset_val >> shift_by
        elif shift_type == 'ROR':
          shift_val = ROR(reg_offset_val, shift_by)
        elif shift_type == 'RRX':
          shift_val = ROR(reg_offset_val, 1) 
          shift_type = 'ROR'

        # print 'shift_val: ', shift_val

        #if shift_type == 'RRX':
          # XXX avoiding RRX for now as this can produce
          # address values (when Carry is 1) causing seg faults.
          # inst += ', ' + 'RRX'
        #else:

        inst += ', ' + shift_type + ' #' + str(shift_by)

    if square_1:
      inst += ']'

    if pre_index:
      inst += '!'

  # print 'inst ', inst
  # print '\n'

  list1.append(inst)
  num_inst += 1

  if cc:
    num_inst += addCondInst(inst, list1, num_inst)

  for e in list1:
    for el in list2:
      list.append(el)
      num_inst += 1
    list.append(e)
    num_inst += 1

  return num_inst




def handle_ARM_extra_load_store_unprivileged(macro, list):
  inst = ''
  num_inst = 0

  list1 = []
  list2 = []

  Rm_reg = ''
  Rn_reg = ''

  match = re.search('([A-Z]{1,6})(<c>)?\s+(<Rt>),\[(<Rn>)\]({)?,(#)?\+/-(<Rm>|<imm8>)', macro)
  if match:
    cmd = match.group(1)
    cc = match.group(2)
    Rt = match.group(3)
    Rn = match.group(4)
    curly = match.group(5)
    hash = match.group(6)
    Rm_or_imm8 = match.group(7)

  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd + ' '

  if Rt:
    Rt_reg = getReg(Rt)
  if Rn:
    while 1:
      Rn_reg = getReg(Rn)
      if Rn_reg != Rt_reg:
        break
    list2.append('LDR ' + Rn_reg + ',' + '=' + choice(['out', 'myarr']))
  
  inst += Rt_reg + ',' + '[' + Rn_reg + ']'

  inst += ','

  if hash:
    inst += '#'

  sign = choice(['+', '-'])
  if sign == '-':
    list2.append('ADD ' + Rn_reg + ',' + '#255')

  inst += sign

  if re.search('Rm', Rm_or_imm8):
    reg_offset_val = random.randint(0, 255)
    while 1:
      Rm_reg = getReg('Rm')
      if Rm_reg != Rn_reg and Rm_reg != Rt_reg:
        break
    list.append('MOV ' + str(Rm_reg) + ',' + str(reg_offset_val))
    inst += Rm_reg
  else:
    inst += getImm('#<imm8>')

  # print 'inst', inst
  # print '\n'

  list1.append(inst)
  num_inst += 1

  if cc:
    num_inst += addCondInst(inst, list1, num_inst)

  for e in list1:
    for el in list2:
      list.append(el)
      num_inst += 1
    list.append(e)
    num_inst += 1
  
  return num_inst    

def handle_ARM_extra_load_store(macro, list):
  inst = ''
  num_inst = 0

  list1 = []
  list2 = []

  match = re.search('([A-Z]{1,6})(<c>)?\s+(<Rt>),(<label>)', macro)

  match1 = re.search('([A-Z]{1,6})(<c>)?\s+(<Rt>),(<Rt2>,)?\[(<Rn>|PC)(\{|\])?,(#)?\+/-(<Rm>|<imm8>)(\})?(\])?(\{!\})?', macro)

  label = ''

  if match:
    cmd = match.group(1)
    cc = match.group(2)
    Rt = match.group(3)
    label = match.group(4)

  elif match1:
    cmd = match1.group(1)
    cc = match1.group(2)
    Rt = match1.group(3)
    Rt2 = match1.group(4)
    Rn_or_pc = match1.group(5)
    curly_square_1 = match1.group(6)
    hash = match1.group(7)
    Rm_or_imm8 = match1.group(8)
    curly_1 = match1.group(9)
    square_1 = match1.group(10)
    pre_index = match1.group(11)  

    # print 'cmd', cmd, 'cc', cc, 'Rt', Rt, 'Rt2', Rt2, 'Rn_or_pc', Rn_or_pc
    # print 'curly_square_1', curly_square_1, 'hash', hash, 'Rm_or_imm8', Rm_or_imm8
    # print 'curly_1', curly_1, 'square_1', square_1, 'pre_index', pre_index
    # print '\n'

  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd + ' '

  reg_Rt = ''
  reg_Rt2 = ''
  reg_Rn = ''
  reg_Rm = ''


  if Rt:
    if cmd[-1] == 'D':
      reg_Rt_num = random.randrange(0,12,2)
      reg_Rt = 'R' + str(reg_Rt_num)
    else:
      reg_Rt = getReg(Rt)
    inst += reg_Rt
  if label:
    inst += ', ' + 'label'
  else:
    if Rt2:
      reg_Rt2 = 'R' + str(reg_Rt_num + 1)
      inst += ',' + reg_Rt2 
    inst += ',' + '['

    if re.search('Rn', Rn_or_pc):
      while 1:
        reg_Rn = getReg('Rn')
        if reg_Rn != reg_Rt and reg_Rn != reg_Rt2:
          break
      list2.append('LDR ' + reg_Rn + ',' + '=' + choice(['out', 'myarr'])) 
      inst += reg_Rn
    elif re.search('PC', Rn_or_pc):
      inst += 'PC'
    
    if curly_square_1:
      if re.search('\]', curly_square_1):
        inst += ']' 
      #elif re.search('{', curly_square_1):
      #  inst += '{' 

    inst += ','

    if hash:
      inst += '#'

    sign = choice(['+', '-'])
    inst += sign
    if sign == '-':
      if reg_Rn:
        list2.append('ADD ' + reg_Rn + ',' + '#255') 

    if Rm_or_imm8:
      if re.search('Rm', Rm_or_imm8):
        reg_offset_val = random.randint(0, 255)
        while(1):
          reg_Rm = getReg('Rm')
          if reg_Rm != reg_Rn and reg_Rm != reg_Rt and reg_Rm != reg_Rt2:
            break
        list.append('MOV ' + str(reg_Rm) + ',' + str(reg_offset_val))
        inst += reg_Rm
      elif re.search('imm8', Rm_or_imm8):
        inst += getImm('#<imm8>')

    #if curly_1:
    #  inst += '}'

    if square_1:
      inst += ']'

    if pre_index:
      inst += '!'

  # print 'inst', inst 
  # print '\n'

  list1.append(inst)
  num_inst += 1

  if cc:
    num_inst += addCondInst(inst, list1, num_inst)

  for e in list1:
    for el in list2:
      list.append(el)
      num_inst += 1
    list.append(e)
    num_inst += 1
  
  return num_inst    

def handle_ARM_misc(macro, list):
  inst = ''
  num_inst = 0

  match = re.search('([A-Z]{1,6})(<c>)?(\s+)?(\S+)?', macro)
  if match:
    cmd = match.group(1)
    cc = match.group(2)
  else:
    print '********** match not found **********'
    sys.exit(0)

  if cmd == 'MRS':
    inst = cmd + ' ' + getReg('Rd') + ',' + choice(['cpsr', 'apsr', 'spsr']) 

  elif cmd == 'CLZ':
    inst = cmd + ' ' + getReg('Rd') + ',' + getReg('Rm')

  # print 'inst', inst

  list.append(inst)
  num_inst += 1

  if cc:
    num_inst += addCondInst(inst, list, num_inst)


  return num_inst

def handle_ARM_MSR_and_hints(macro, list):
  inst = ''
  num_inst = 0

  match = re.search('([A-Z]{1,6})(<c>)?(\s+)?(\S+)?', macro)
  if match:
    cmd = match.group(1)
    cc = match.group(2)
    opt = match.group(4)
    # print 'cmd', cmd, 'cc', cc, 'opt', opt
    # print '\n'
  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd +' '

  if cmd == 'MSR':
    inst += 'apsr_nzcvq, #' + hex(random.randint(0,255)) + '000000'

  elif cmd == 'DBG':
    inst += str(random.randint(0, 15))

  # print 'inst:', inst
  list.append(inst)
  num_inst += 1

  if cc:
    num_inst += addCondInst(inst, list, num_inst)


  return num_inst

def handle_ARM_halfword_multiply_and_multiplyaccumulate(macro, list):
  inst = ''
  num_inst = 0
 
  match = re.search('([A-Z]{1,6})(<x>)?(<y>)?(<c>)?\s+(<Rd>)?(<RdLo>)?(,<RdHi>)?(,<Rn>)?(,<Rm>)?(,<Ra>)?', macro)
  if match:
    cmd = match.group(1)
    x = match.group(2)
    y = match.group(3)
    cc = match.group(4)
    Rd = match.group(5)
    RdLo = match.group(6)
    RdHi = match.group(7)
    Rn = match.group(8)
    Rm = match.group(9)
    Ra = match.group(10)
    # print 'cmd', cmd, 'x', x, 'y', y, 'cc', cc, 'Rd', Rd, 'Rn', Rn, 'Rm', Rm, 'Ra', Ra, 'RdLo', RdLo, 'RdHi', RdHi
    # print '\n'
  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd

  if x:
    inst += getXY()
  if y:
    inst += getXY()

  inst += ' '

  if RdLo:
    rd_lo = getReg(RdLo)
    inst += rd_lo + ', '
  if RdHi:
    while 1:
      rd_hi = getReg(RdHi)
      if rd_hi != rd_lo: 
        inst += rd_hi
        break

  if Rd:
    inst += getReg(Rd)
  if Rn:
    inst += ', ' + getReg(Rn)
  if Rm:
    inst += ', ' + getReg(Rm)
  if Ra:
    inst += ', ' + getReg(Ra)

  # print 'inst:', inst
  list.append(inst)
  num_inst += 1

  if cc:
    num_inst += addCondInst(inst, list, num_inst)

  return num_inst

def handle_ARM_saturating_addition_and_subtraction(macro, list):
  inst = ''
  num_inst = 0

  match = re.search('([A-Z]{1,6})(<c>)?\s+(<Rd>),(<Rm>),(<Rn>)', macro)
  if match:
    cmd = match.group(1)
    cc = match.group(2)
    Rd = match.group(3)
    Rm = match.group(4)
    Rn = match.group(5)
    # print 'cmd', cmd, 'cc', cc, 'Rd', Rd, 'Rm', Rm, 'Rn', Rn
    # print '\n'
  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd + ' ' + getReg(Rd) + ',' + getReg(Rm) + ',' + getReg(Rn)

  # print 'inst', inst
  list.append(inst)
  num_inst += 1

  if cc:
    num_inst += addCondInst(inst, list, num_inst)
  
  return num_inst

def handle_ARM_multiply_and_multiplyaccumulate(macro, list):
  inst = ''
  num_inst = 0

  match = re.search('([A-Z]{1,6})(\{S\})?(<c>)?\s+(<RdLo>,)?(<RdHi>,)?(<Rd>,)?(<Rn>,)?(<Rm>)?,?(<Ra>)?', macro)
  if match:
    cmd = match.group(1)
    set_icc = match.group(2)
    cc = match.group(3)
    RdLo = match.group(4)
    RdHi = match.group(5)
    Rd = match.group(6)
    Rn = match.group(7)
    Rm = match.group(8)
    Ra = match.group(9)
    # print 'cmd', cmd, 'set_icc', set_icc, 'cc', cc, 'RdLo', RdLo, 'RdHi', RdHi, 'Rd', Rd, 'Rn', Rn, 'Rm', Rm, 'Ra', Ra
    # print '\n'
  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd + ' '

  if RdLo:
    rd_lo = getReg(RdLo)
    inst += rd_lo + ', '
  if RdHi:
    while 1:
      rd_hi = getReg(RdHi)
      if rd_hi != rd_lo: 
        inst += rd_hi + ', '
        break
  if Rd:
    inst += getReg(Rd) + ', '
  if Rn:
    inst += getReg(Rn) + ', '
  if Rm:
    inst += getReg(Rm)
  if Ra:
    inst += ', ' + getReg(Ra)

  # print 'inst', inst
  list.append(inst)
  num_inst += 1

  if set_icc:
    num_inst += addSetCondCodesInst(inst, list, num_inst)

  if cc:
    num_inst += addCondInst(inst, list, num_inst)

  return num_inst

def handle_ARM_data_processing_immediate(macro, list):
  inst = ''
  num_inst = 0

  match = re.search('([A-Z]{1,6})(\{S\})?(<c>)?\s+(<Rd>)?,?(<Rn>)?,?(#<const>)?(#<imm16>)?', macro)
  if match:
    cmd = match.group(1)
    set_icc = match.group(2)
    cc = match.group(3)
    Rd = match.group(4)
    Rn = match.group(5)
    const = match.group(6)
    imm16 = match.group(7)
    # print 'cmd', cmd, 'set_icc', set_icc, 'cc', cc, 'Rd', Rd, 'Rn', Rn, 'const', const, 'imm16', imm16
  else:
    print '********** match not found **********'
    sys.exit(0)

  # XXX Handle ADR later
  if re.search('ADR', cmd):
    list.append('ADR')
    num_inst += 1
    return num_inst

  inst += cmd + ' '

  if Rd:
    inst += getReg(Rd)
  if Rn:
    if Rd:
      inst += ','
    inst += getReg(Rn) 

  if const:
    inst += ',' + ' #' + getConst()	

  if imm16:
    inst += ',' + getImm(imm16)

  # print 'inst: ', inst
  list.append(inst)
  num_inst += 1

  if set_icc:
    num_inst += addSetCondCodesInst(inst, list, num_inst)

  if cc:
    num_inst += addCondInst(inst, list, num_inst)

  return num_inst

def handle_ARM_data_processing_register_shifted_register(macro, list):
  inst = ''
  num_inst = 0

  match = re.search('([A-Z]{1,3})(\{S\})?(<c>)?\s+(<Rd>)?,?(<Rn>)?,?(<Rm>),?(<type>)?\s?(<Rs>)?', macro)
  if match:
    cmd = match.group(1)
    set_icc = match.group(2)
    cc = match.group(3)
    Rd = match.group(4)
    Rn = match.group(5)
    Rm = match.group(6)
    type = match.group(7)
    Rs = match.group(8)
    # print 'cmd', cmd, 'set_icc', set_icc, 'cc', cc, 'Rd', Rd, 'Rn', Rn, 'Rm', Rm, 'type', type, 'Rs', Rs

  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd + ' '

  if Rd:
    inst += getReg(Rd)
  if Rn:
    if Rd:
      inst += ','
    inst += getReg(Rn) 
  if Rm:
    inst += ',' + getReg(Rm) 

  if type:
    s = getType(type)
    if s:
      shift_val = random.randint(0, 255)
      shift_reg = 'R' + str(random.randint(0, 12))
      list.append('MOV %s, #%s \n' % (shift_reg, shift_val))
      num_inst += 1
      inst += ',' + s + ' ' + shift_reg

  # print 'inst:', inst
  list.append(inst)
  num_inst += 1

  if set_icc:
    num_inst += addSetCondCodesInst(inst, list, num_inst)

  if cc:
    num_inst += addCondInst(inst, list, num_inst)

  return num_inst


def handle_ARM_data_processing_register(macro, list):
  inst = ''
  num_inst = 0

  match = re.search('([A-Z]{1,3})(\{S\})?(<c>)?\s+(<Rd>)?,?(<Rn>)?,?(<Rm>)(\{,<shift>\})?,?(#<imm5>)?', macro)
  if match:
    cmd = match.group(1)
    set_icc = match.group(2)
    cc = match.group(3)
    Rd = match.group(4)
    Rn = match.group(5)
    Rm = match.group(6)
    shift = match.group(7)
    imm5 = match.group(8)
    # print 'cmd', cmd, 'set_icc', set_icc, 'cc', cc, 'Rd', Rd, 'Rn', Rn, 'Rm', Rm, 'shift', shift, 'imm5', imm5

  else:
    print '********** match not found **********'
    sys.exit(0)

  inst += cmd + ' '

  if Rd:
    inst += getReg(Rd)
  if Rn:
    if Rd:
      inst += ','
    inst += getReg(Rn) 
  if Rm:
    inst += ',' + getReg(Rm) 
  if shift:
    s = getShift(shift)
    if(s):
      inst += ',' + s
  if imm5:
    inst += ',' + getImm(imm5)

  # print 'inst:', inst
  list.append(inst)
  num_inst += 1

  if set_icc:
    num_inst += addSetCondCodesInst(inst, list, num_inst)

  if cc:
    num_inst += addCondInst(inst, list, num_inst)

  return num_inst

def parseAndGenerateInst(macro, type, list):

  if type == 'ARM_data_processing_register':
    num_inst = handle_ARM_data_processing_register(macro, list)
  if type == 'ARM_data_processing_register_shifted_register':
    num_inst = handle_ARM_data_processing_register_shifted_register(macro, list)
  if type == 'ARM_data_processing_immediate':
    num_inst = handle_ARM_data_processing_immediate(macro, list) 
  if type == 'ARM_multiply_and_multiplyaccumulate':
    num_inst = handle_ARM_multiply_and_multiplyaccumulate(macro, list) 
  if type == 'ARM_saturating_addition_and_subtraction':
    num_inst = handle_ARM_saturating_addition_and_subtraction(macro, list)
  if type == 'ARM_halfword_multiply_and_multiplyaccumulate':
    num_inst = handle_ARM_halfword_multiply_and_multiplyaccumulate(macro, list)
  if type == 'ARM_MSR_and_hints':
    num_inst = handle_ARM_MSR_and_hints(macro, list)
  if type == 'ARM_misc':
    num_inst = handle_ARM_misc(macro, list)
  if type == 'ARM_extra_load_store':
    num_inst = handle_ARM_extra_load_store(macro, list)
  if type == 'ARM_extra_load_store_unprivileged':
    num_inst = handle_ARM_extra_load_store_unprivileged(macro, list)
  if type == 'ARM_load_store_word_unsigned_byte':
    num_inst = handle_ARM_load_store_word_and_unsigned_byte(macro, list)
  if type == 'ARM_branch_branch_with_link_block_data_transfer':
    num_inst = handle_ARM_branch_branch_with_link_block_data_transfer(macro, list)

  return num_inst
