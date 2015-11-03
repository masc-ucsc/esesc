# Written by Hadley Black
#   ---for regression test suite 

# cmp_tab.py
# parses 2 tables, comparing the following metrics
# Exe, IPC, uIPC, rawInst
# results go to:
# example call:

#   python cmp_tab_hb.py infile1 infile2 outfile 
#                        -t title (FAST, SLOW, etc...)
#                        -m num_metrics (assumes 4 if not specified)
#                        -e eps_1 ... eps_n
#                        -c expec_cmp_1 ... expec_cmp_n (expects "eq, ne, gt, lt, ge, le) 



import sys, fileinput, string 

num_metrics = 4
eps_list = [] # holds allowed diffs for metrics        [exe, ipc, uipc, raw] in that order
exp_list = [] # holds expected cmp results for metrics [exe, ipc, uipc, raw] in that order
res_list = [] # holds actual   cmp results for metrics ...
err_list = [] # holds either OK or ERROR depending on whether exp_list[i] == res_list[i]

i = 1
while i < len(sys.argv):
  if(i == 1): in1_name  = sys.argv[i]
  if(i == 2): in2_name  = sys.argv[i]
  if(i == 3): out_name  = sys.argv[i]
  if(sys.argv[i] == "-t"): title = sys.argv[i + 1] 
  if(sys.argv[i] == "-m"): num_metrics = sys.argv[i + 1]
  if(sys.argv[i] == "-e"):
    i = i + 1
    j = 0
    while j < num_metrics:
      eps_list.append(float(sys.argv[i + j]))
      j = j + 1

  if(sys.argv[i] == "-c"):
    i = i + 1
    j = 0
    while j < num_metrics:
      exp_list.append(sys.argv[i + j])
      j = j + 1
  
  i = i + 1

i_f1 = open(in1_name, "r")
i_f2 = open(in2_name, "r")
out  = open(out_name, "w+")

# exe_1, ipc_1, uipc_1, raw_1 # metrics from file 1
# exe_2, ipc_2, uipc_2, raw_2 # metrics from file 2


# parse input file 1
for i, line in enumerate(i_f1):
  if i == 1:
    
    # get exe_1
    j = 0
    while j < len(line) and line[j] != ' ': j = j + 1
    j = j + 1

    k = j
    while k < len(line) and line[k] != ',': k = k + 1
    
    exe_1 = line[j:k]

    # get ipc_1
    j = k + 1
    while j < len(line) and line[j] != ' ': j = j + 1
    j = j + 1

    k = j
    while k < len(line) and line[k] != ',': k = k + 1
    
    ipc_1 = line[j:k]

    # get uipc_1
    j = k + 1
    while j < len(line) and line[j] != ' ': j = j + 1
    j = j + 1

    k = j
    while k < len(line) and line[k] != ',': k = k + 1
    
    uipc_1 = line[j:k]

    # get raw_1
    j = k + 1
    while j < len(line) and line[j] != ' ': j = j + 1
    j = j + 1

    k = j
    while k < len(line) and line[k] != '\n': k = k + 1
    
    raw_1 = line[j:k]

# parse input file 2
for i, line in enumerate(i_f2):
  if i == 1:
    
    # get exe_2
    j = 0
    while j < len(line) and line[j] != ' ': j = j + 1
    j = j + 1

    k = j
    while k < len(line) and line[k] != ',': k = k + 1
    
    exe_2 = line[j:k]

    # get ipc_2
    j = k + 1
    while j < len(line) and line[j] != ' ': j = j + 1
    j = j + 1

    k = j
    while k < len(line) and line[k] != ',': k = k + 1
    
    ipc_2 = line[j:k]

    # get uipc_2
    j = k + 1
    while j < len(line) and line[j] != ' ': j = j + 1
    j = j + 1

    k = j
    while k < len(line) and line[k] != ',': k = k + 1
    
    uipc_2 = line[j:k]

    # get raw_2
    j = k + 1
    while j < len(line) and line[j] != ' ': j = j + 1
    j = j + 1

    k = j
    while k < len(line) and line[k] != '\n': k = k + 1
    
    raw_2 = line[j:k]


# compute differences 
exe_1_value = float(exe_1)
exe_2_value = float(exe_2)
exe_diff  = ( exe_1_value - exe_2_value)

ipc_1_value = float(ipc_1)
ipc_2_value = float(ipc_2)
ipc_diff  = ( ipc_1_value - ipc_2_value)

uipc_1_value = float(uipc_1)
uipc_2_value = float(uipc_2)
uipc_diff  = ( uipc_1_value - uipc_2_value)

raw_1_value = float(raw_1)
raw_2_value = float(raw_2)
raw_diff  = ( raw_1_value - raw_2_value)


if(abs(exe_diff) <= eps_list[0]): res_list.append("eq")
elif(exe_diff > 0): res_list.append("gt")
elif(exe_diff < 0): res_list.append("lt")

if(abs(ipc_diff) <= eps_list[1]): res_list.append("eq")
elif(ipc_diff > 0): res_list.append("gt")
elif(ipc_diff < 0): res_list.append("lt")

if(abs(uipc_diff) <= eps_list[2]): res_list.append("eq")
elif(uipc_diff > 0): res_list.append("gt")
elif(uipc_diff < 0): res_list.append("lt")

if(abs(raw_diff) <= eps_list[3]): res_list.append("eq")
elif(raw_diff > 0): res_list.append("gt")
elif(raw_diff < 0): res_list.append("lt")

# find if there are ERRORs
if(res_list[0] == exp_list[0]): err_list.append("OK")
else: err_list.append("ERROR")

if(res_list[1] == exp_list[1]): err_list.append("OK")
else: err_list.append("ERROR")

if(res_list[2] == exp_list[2]): err_list.append("OK")
else: err_list.append("ERROR")

if(res_list[3] == exp_list[3]): err_list.append("OK")
else: err_list.append("ERROR")

i_f1.seek(0)

# write title
out.write(out_name + "\n\n")

# write table 1
out.write("Input table 1: \n")
for line in i_f1:
   out.write(line)

out.write("\n")
i_f2.seek(0)

# write table 2
out.write("Input table 2: \n")
for line in i_f2:
   out.write(line)

# write Diffs
out.write("\nDifferences: \n")
out.write("Exe, IPC, uIPC, rawInst\n")
out.write(str(exe_diff) + ", " + str(ipc_diff) + ", " + str(uipc_diff) + ", " + str(raw_diff) + "\n\n") 

# write ERRORs
out.write("Errors: \n")
out.write("Exe, IPC, uIPC, rawInst\n")
out.write(err_list[0] + ", " + err_list[1] + ", " + err_list[2] + ", " + err_list[3] + "\n\n") 

# write epsilon values and expected compare parameters 
out.write("Parameters: \n")
out.write("Exe, IPC, uIPC, rawInst\n")
out.write("Epsilon, " + str(eps_list[0]) + ", " + str(eps_list[1]) + ", " + str(eps_list[2]) + ", " + str(eps_list[3]) + "\n") 
out.write("Expected, " + exp_list[0] + ", " + exp_list[1] + ", " + exp_list[2] + ", " + exp_list[3] + "\n\n") 



