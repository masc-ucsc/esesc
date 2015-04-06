# Written by Hadley Black
# ---for regression test suite

# mk_table.py
# parses a report file grabbing the metrics listed below and putting them in a table:
# Exe, IPC, uIPC, rawInst
# resulting table goes to build/regression/tables and $esesc_dir/gold/tables
# example call:

    #              $4/gold/exe/mk_table_hb.py report.$1.$BENCHMARK $1.$date.crafty       #

# currently does not account for multicore benchmarks (i.e assumes crafty/mcf/vpr)  

import sys, fileinput, string 

in_file_name  = sys.argv[1]
out_file_name = sys.argv[2]

in_file = open(in_file_name, "r")
out_file = open(out_file_name, "a")

str1 = ""
IPC = 0; # flag for IPC and uIPC line
rawInst = 0; # flag for rawInst

benchmark = in_file_name[in_file_name.find("."):]
benchmark = benchmark[1:]
benchmark = benchmark[benchmark.find("."):]
benchmark = benchmark[1:]

for line in fileinput.input():

   # find Exe
   if " Exe " in line:
     i = line.find("Exe")
     str1 = line[i + 3:]

     i = 0;
     while i < len(str1) and str1[i] == ' ': i = i + 1

     str1 = str1[i:]

     i = 0;
     while i < len(str1) and str1[i] != ' ': i = i + 1
 
     str1 = str1[:i]
     exe = str1  

   # find line containing IPC
   if " IPC " in line:
     j = line.find("IPC")
     IPC = 1; 
     continue;

   # line after IPC is found
   if IPC == 1:
     str1 = line[j:]
     
     i = 0;
     while i < len(str1) and str1[i] != ' ': i = i + 1

     ipc = str1[:i]
     # finished finding ipc 
     
     uIpc = str1[i + 1:]

     i = 0;
     while i < len(uIpc) and uIpc[i] != ' ': i = i + 1

     uIpc = uIpc[:i]
    
     IPC = 0


   # find line containing rawInst
   if " rawInst " in line:
     j = line.find("rawInst")
     rawInst = 1; 
     continue;


   # line after rawInst is found
   if rawInst == 1:
     str1 = line[j:]
     
     i = 0
     while i < len(str1) and str1[i] == ' ': i = i + 1
     
     str1 = str1[i:]
     
     i = 0
     while i < len(str1) and str1[i] != ' ': i = i + 1

     str1 = str1[:i]

     raw = str1

     rawInst = 0


# write results in correct order
out_file.write(" "  +  exe)
out_file.write(", " +  ipc)
out_file.write(", " + uIpc)
out_file.write(", " +  raw)
