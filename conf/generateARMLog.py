#! /usr/bin/python

import pexpect
import sys
import re

#
# Some constants
#
COMMAND_PROMPT = '\(gdb\)'
EXIT_MESSAGE = 'exited with code 01'
PREV_REG_LIST_FOR_COMPARE = []

def main():

  global COMMAND_PROMPT, EXIT_MESSAGE

  if (len(sys.argv) < 2):
    print("\n")
    print("Usage:   generateARMLog.py [-t] binary")
    print(" It should generate a binary.stack_XXX and binary.trace. With sccore \n")
    print("  generateARMLog.py ./hello_diet\n")
    print("  ./main/scmain 0xbeffe980 0xbeffe000 ./kernel/hello_diet.stack_0xbeffe980 ./kernel/hello_diet\n")
    print("\n")
    sys.exit(1)

  dotrace = 0
  binary = sys.argv[1]
  if sys.argv[1] == '-t':
    dotrace = 1
    binary  = sys.argv[2]

  out_file = '%s.trace' % binary
  # print(out_file);

  # XXX Binary should be input
  child = pexpect.spawn('gdb -quiet %s' % binary)
  child.logfile = sys.stdout
  i = child.expect([COMMAND_PROMPT, pexpect.TIMEOUT])

  if i == 0:
    pass
  if i == 1: # Timeout
    print('ERROR! could not attach gdb. Here is what GDB said:')
    print(child.before, child.after)
    print(str(child))
    sys.exit (1)

  child.sendline('b *_start')  
  child.expect(COMMAND_PROMPT)  
  child.sendline('run')  
  child.expect(COMMAND_PROMPT)  

  # Generate the stack file
  child.sendline('p /x $sp')  
  child.expect(COMMAND_PROMPT)  
  sp = parseForSP(child.before)
  print('stack starts at %s' % sp)
  child.sendline('dump binary memory %s.stack_%s %s %s' % (binary, sp, hex(int(sp,16) & 0xFFFFF000), hex(int(sp,16) | 0x0000FFF)))
  child.expect(COMMAND_PROMPT)

  print("scmain usage:\n")
  print("\t./main/scmain %s %s %s.stack_%s %s\n" % (sp, hex(int(sp,16) & 0xFFFFF000) ,binary, sp, binary))
  if dotrace == 0:
     exit(0)

  print("Starting trace generation...\n")

  fd = open(out_file, 'w')
  # FIXME: for the moment, let's just try 1000 instructions
  for i in range(1000):
    child.sendline('p /x $pc')
    child.expect(COMMAND_PROMPT)
    pc = parseForPC(child.before)
    fd.write("pc: ")
    fd.write(pc)
    fd.write('\n')

    nextPc = calculatePcPlusFour(pc)
    sndline = 'disas /r ' + pc + ',' + nextPc

    #child.sendline('disas /r %s, %s' % (pc, nextPc))
    child.sendline(sndline)
    child.expect(COMMAND_PROMPT)
    processInstruction(child.before, fd)

    child.sendline('si')
    i = child.expect([EXIT_MESSAGE, COMMAND_PROMPT])

    if i == 0: # Finished GDB
      child.sendline('quit')
      child.expect(pexpect.EOF)
      break
    if i == 1:
      child.sendline('info registers')  
      child.expect(COMMAND_PROMPT)  
      #fd.write(child.before)
      processRegisterInfo(child.before, fd)

  fd.close()

def parseForPC(out):
    pc = 0
    for line in out.split('\n'):
        match = re.match('.+ = (0x\w+)', line)
        if match:
            pc = match.group(1)
        return pc

    if pc == 0:
        print('ERROR! No PC address found in %s' %out)
        sys.exit(1)


def parseForSP(out):
  sp = 0
  for line in out.split('\n'):
    match = re.match('.+ = (0x\w+)', line)
    if match:
      sp = match.group(1)
      return sp

  if sp == 0:
    print('ERROR! No SP address found in %s' %out)
    sys.exit(1)


def calculatePcPlusFour(pc):
  return hex(int(pc, 16) + 4)


def processInstruction(out, fd):
  first = 0
  fd.write('inst: 0x')
  for line in out.split('\n'):
    # print 'Line: %s' %line
    match = re.search(r':\s+([0-9a-zA-Z][0-9a-zA-Z]) ([0-9a-zA-Z][0-9a-zA-Z]) ([0-9a-zA-Z][0-9a-zA-Z]) ([0-9a-zA-Z][0-9a-zA-Z])', line)
    if match:
      byte4 = match.group(1)
      byte3 = match.group(2)
      byte2 = match.group(3)
      byte1 = match.group(4)
      fd.write(byte1)
      fd.write(byte2)
      fd.write(byte3)
      fd.write(byte4)
      fd.write('\n')
      return

    match = re.search(r':\s+([0-9a-zA-Z][0-9a-zA-Z]) ([0-9a-zA-Z][0-9a-zA-Z])', line)
    if match:
      if first == 0:
        byte4 = match.group(1)
        byte3 = match.group(2)
        first = 1
      if first == 1:
        byte2 = match.group(1)
        byte1 = match.group(2)


  fd.write(byte1)
  fd.write(byte2)
  fd.write(byte3)
  fd.write(byte4)
  fd.write('\n')
  # if found == 0:
  #   print 'match not found'

def processRegisterInfo(out, fd):

  global PREV_REG_LIST_FOR_COMPARE

  for line in out.split('\n'):
    match = re.search(r'r([0-9]+)\s+(0x\w+)', line)
    if match:
      reg_sub_num = int(match.group(1))
      reg_val = match.group(2)

      if len(PREV_REG_LIST_FOR_COMPARE) >= 13:
        # Compare previous and current register value
        if PREV_REG_LIST_FOR_COMPARE[reg_sub_num] != reg_val:
          # Register modified. Write to the log file and update PREV_REG.. list

          # APP reg number starts from 1, ARM reg number starts from 0 
          # Before printing in out_file, increment ARM reg number so that comparison 
          # will be easier later.
          incr_reg_sub_num = reg_sub_num + 1
          fd.write('r')
          fd.write(str(incr_reg_sub_num))
          fd.write('=')
          fd.write(reg_val)
          fd.write('\n')
          PREV_REG_LIST_FOR_COMPARE[reg_sub_num] = reg_val
      else:
        PREV_REG_LIST_FOR_COMPARE.append(reg_val)
        incr_reg_sub_num = reg_sub_num + 1
        fd.write('r')
        fd.write(str(incr_reg_sub_num))
        fd.write('=')
        fd.write(reg_val)
        fd.write('\n')

    match = re.search(r'sp\s+(0x\w+)', line)
    if match:
      reg_sub_num = 13
      reg_val = match.group(1)

      if len(PREV_REG_LIST_FOR_COMPARE) >= 14:
        if PREV_REG_LIST_FOR_COMPARE[reg_sub_num] != reg_val:
          # Register modified
          incr_reg_sub_num = reg_sub_num + 1
          fd.write('r')
          fd.write(str(incr_reg_sub_num))
          fd.write('=')
          fd.write(reg_val)
          fd.write('\n')
          PREV_REG_LIST_FOR_COMPARE[reg_sub_num] = reg_val
      else:
        PREV_REG_LIST_FOR_COMPARE.append(reg_val)
        incr_reg_sub_num = reg_sub_num + 1
        fd.write('r')
        fd.write(str(incr_reg_sub_num))
        fd.write('=')
        fd.write(reg_val)
        fd.write('\n')
    
    match = re.search(r'lr\s+(0x\w+)', line)
    if match:
      reg_sub_num = 14
      reg_val = match.group(1)

      if len(PREV_REG_LIST_FOR_COMPARE) == 15:
        if PREV_REG_LIST_FOR_COMPARE[reg_sub_num] != reg_val:
          # Register modified
          incr_reg_sub_num = reg_sub_num + 1
          fd.write('r')
          fd.write(str(incr_reg_sub_num))
          fd.write('=')
          fd.write(reg_val)
          fd.write('\n')
          PREV_REG_LIST_FOR_COMPARE[reg_sub_num] = reg_val
      else:
        PREV_REG_LIST_FOR_COMPARE.append(reg_val)
        incr_reg_sub_num = reg_sub_num + 1
        fd.write('r')
        fd.write(str(incr_reg_sub_num))
        fd.write('=')
        fd.write(reg_val)
        fd.write('\n')
  fd.write("----------------")
  fd.write('\n')


main()

