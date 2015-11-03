from __future__ import division
import math
import datetime
import sys

rightnow = datetime.datetime.now() 

build_Addr = sys.argv[1]
proj_Addr  = sys.argv[2]

regression_tables_Dir  = build_Addr+"/regression/tables/"
report_Dir             = build_Addr+"/regression/report/"
gold_tables_Dir  = build_Addr+"/gold/tables/"

def trunc(f):
    slen = len('%.*f' % (1, f))
    return str(f)[:slen]

def trunc3(f):
    slen = len('%.*f' % (3, f))
    return str(f)[:slen]

def dzer(a,b):
    if b==0:
       return 0
    else: 
       return a/b 

def AVG(my_list):
    sum_of_grades = sum(my_list)
    average = dzer(sum_of_grades, len(my_list))
    return average

file_memkernels_stream  = open (report_Dir+"report_memkernels_stream.txt", "r") 
lines_memkernels_stream = file_memkernels_stream.readlines()
file_memkernels_stream.close()
	
file_memkernels_stream_omp = open (report_Dir+"report_memkernels_stream_omp.txt", "r")
lines_memkernels_stream_omp = file_memkernels_stream_omp.readlines()
file_memkernels_stream_omp.close()

file_launcher_blackscholes=open(report_Dir+"report_launcher_blackscholes.txt","r")
lines_launcher_blackscholes=file_launcher_blackscholes.readlines() 
file_launcher_blackscholes.close()

file_launcher_fft = open (report_Dir+"report_launcher_fft.txt","r") 
lines_launcher_fft = file_launcher_fft.readlines()
file_launcher_fft.close()	


input_max_table_file1 = open(gold_tables_Dir+"Max_Dev_Percentage_table1.txt","r")
input_val_table_file1 = open(gold_tables_Dir+"Gold_Avg_Value_table1.txt","r")
input_max_table_file2 = open(gold_tables_Dir+"Max_Dev_Percentage_table2.txt","r")
input_val_table_file2 = open(gold_tables_Dir+"Gold_Avg_Value_table2.txt","r")

input_lines = input_max_table_file1.readlines()
Exe_Stream_Perc1         = float(input_lines[1].split("'")[1])
Exe_Stream_omp_Perc1     = float(input_lines[1].split("'")[3])
Exe_blackscholes_Perc1   = float(input_lines[1].split("'")[5])
Exe_fft_Perc1            = float(input_lines[1].split("'")[7])

MisBr_Stream_Perc1         = float(input_lines[2].split("'")[1])
MisBr_Stream_omp_Perc1     = float(input_lines[2].split("'")[3])
MisBr_blackscholes_Perc1   = float(input_lines[2].split("'")[5])
MisBr_fft_Perc1            = float(input_lines[2].split("'")[7])

IPC_Stream_Perc1         = float(input_lines[3].split("'")[1])
IPC_Stream_omp_Perc1     = float(input_lines[3].split("'")[3])
IPC_blackscholes_Perc1   = float(input_lines[3].split("'")[5])
IPC_fft_Perc1            = float(input_lines[3].split("'")[7])

uIPC_Stream_Perc1         = float(input_lines[4].split("'")[1])
uIPC_Stream_omp_Perc1     = float(input_lines[4].split("'")[3])
uIPC_blackscholes_Perc1   = float(input_lines[4].split("'")[5])
uIPC_fft_Perc1            = float(input_lines[4].split("'")[7])

rawInst_Stream_Perc1         = float(input_lines[5].split("'")[1])
rawInst_Stream_omp_Perc1     = float(input_lines[5].split("'")[3])
rawInst_blackscholes_Perc1   = float(input_lines[5].split("'")[5])
rawInst_fft_Perc1            = float(input_lines[5].split("'")[7])
input_max_table_file1.close()

input_lines = input_val_table_file1.readlines()
Exe_Stream_val1         = float(input_lines[1].split("'")[1])
Exe_Stream_omp_val1     = float(input_lines[1].split("'")[3])
Exe_blackscholes_val1   = float(input_lines[1].split("'")[5])
Exe_fft_val1            = float(input_lines[1].split("'")[7])

MisBr_Stream_val1         = float(input_lines[2].split("'")[1])
MisBr_Stream_omp_val1     = float(input_lines[2].split("'")[3])
MisBr_blackscholes_val1   = float(input_lines[2].split("'")[5])
MisBr_fft_val1            = float(input_lines[2].split("'")[7])

IPC_Stream_val1         = float(input_lines[3].split("'")[1])
IPC_Stream_omp_val1     = float(input_lines[3].split("'")[3])
IPC_blackscholes_val1   = float(input_lines[3].split("'")[5])
IPC_fft_val1            = float(input_lines[3].split("'")[7])

uIPC_Stream_val1         = float(input_lines[4].split("'")[1])
uIPC_Stream_omp_val1     = float(input_lines[4].split("'")[3])
uIPC_blackscholes_val1   = float(input_lines[4].split("'")[5])
uIPC_fft_val1            = float(input_lines[4].split("'")[7])

rawInst_Stream_val1         = float(input_lines[5].split("'")[1])
rawInst_Stream_omp_val1     = float(input_lines[5].split("'")[3])
rawInst_blackscholes_val1   = float(input_lines[5].split("'")[5])
rawInst_fft_val1            = float(input_lines[5].split("'")[7])
input_val_table_file1.close()

input_lines = input_max_table_file2.readlines()
Exe_Stream_Perc2         = float(input_lines[1].split("'")[1])
Exe_Stream_omp_Perc2     = float(input_lines[1].split("'")[3])
Exe_blackscholes_Perc2   = float(input_lines[1].split("'")[5])
Exe_fft_Perc2            = float(input_lines[1].split("'")[7])

MisBr_Stream_Perc2         = float(input_lines[2].split("'")[1])
MisBr_Stream_omp_Perc2     = float(input_lines[2].split("'")[3])
MisBr_blackscholes_Perc2   = float(input_lines[2].split("'")[5])
MisBr_fft_Perc2            = float(input_lines[2].split("'")[7])

IPC_Stream_Perc2         = float(input_lines[3].split("'")[1])
IPC_Stream_omp_Perc2     = float(input_lines[3].split("'")[3])
IPC_blackscholes_Perc2   = float(input_lines[3].split("'")[5])
IPC_fft_Perc2            = float(input_lines[3].split("'")[7])

uIPC_Stream_Perc2         = float(input_lines[4].split("'")[1])
uIPC_Stream_omp_Perc2     = float(input_lines[4].split("'")[3])
uIPC_blackscholes_Perc2   = float(input_lines[4].split("'")[5])
uIPC_fft_Perc2            = float(input_lines[4].split("'")[7])

rawInst_Stream_Perc2         = float(input_lines[5].split("'")[1])
rawInst_Stream_omp_Perc2     = float(input_lines[5].split("'")[3])
rawInst_blackscholes_Perc2   = float(input_lines[5].split("'")[5])
rawInst_fft_Perc2            = float(input_lines[5].split("'")[7])
input_max_table_file2.close()

input_lines = input_val_table_file2.readlines()
Exe_Stream_val2         = float(input_lines[1].split("'")[1])
Exe_Stream_omp_val2     = float(input_lines[1].split("'")[3])
Exe_blackscholes_val2   = float(input_lines[1].split("'")[5])
Exe_fft_val2            = float(input_lines[1].split("'")[7])

MisBr_Stream_val2         = float(input_lines[2].split("'")[1])
MisBr_Stream_omp_val2     = float(input_lines[2].split("'")[3])
MisBr_blackscholes_val2   = float(input_lines[2].split("'")[5])
MisBr_fft_val2            = float(input_lines[2].split("'")[7])

IPC_Stream_val2         = float(input_lines[3].split("'")[1])
IPC_Stream_omp_val2     = float(input_lines[3].split("'")[3])
IPC_blackscholes_val2   = float(input_lines[3].split("'")[5])
IPC_fft_val2            = float(input_lines[3].split("'")[7])

uIPC_Stream_val2         = float(input_lines[4].split("'")[1])
uIPC_Stream_omp_val2     = float(input_lines[4].split("'")[3])
uIPC_blackscholes_val2   = float(input_lines[4].split("'")[5])
uIPC_fft_val2            = float(input_lines[4].split("'")[7])

rawInst_Stream_val2         = float(input_lines[5].split("'")[1])
rawInst_Stream_omp_val2     = float(input_lines[5].split("'")[3])
rawInst_blackscholes_val2   = float(input_lines[5].split("'")[5])
rawInst_fft_val2            = float(input_lines[5].split("'")[7])
input_val_table_file2.close()

if len(lines_memkernels_stream)==32: 
	Exe_Stream_val             = Exe_Stream_val1
	MisBr_Stream_val           = MisBr_Stream_val1
	IPC_Stream_val             = IPC_Stream_val1 
	uIPC_Stream_val	           = uIPC_Stream_val1 
	rawInst_Stream_val         = rawInst_Stream_val1
	Exe_Stream_Perc            = Exe_Stream_Perc1
        MisBr_Stream_Perc          = MisBr_Stream_Perc1
        IPC_Stream_Perc            = IPC_Stream_Perc1
        uIPC_Stream_Perc           = uIPC_Stream_Perc1
        rawInst_Stream_Perc        = rawInst_Stream_Perc1
        Exe_memkernels_stream      = float(lines_memkernels_stream[8].split()[9])
	MisBr_memkernels_stream    = float(lines_memkernels_stream[19].split()[13])
	IPC_memkernels_stream      = float(lines_memkernels_stream[19].split()[1])
	uIPC_memkernels_stream     = float(lines_memkernels_stream[19].split()[2])
	rawInst_memkernels_stream  = float(lines_memkernels_stream[15].split()[2])
elif len(lines_memkernels_stream)==39:  
	Exe_Stream_val             = Exe_Stream_val2
	MisBr_Stream_val           = MisBr_Stream_val2
	IPC_Stream_val             = IPC_Stream_val2 
	uIPC_Stream_val	           = uIPC_Stream_val2 
	rawInst_Stream_val         = rawInst_Stream_val2 
        Exe_Stream_Perc            = Exe_Stream_Perc2
        MisBr_Stream_Perc          = MisBr_Stream_Perc2
        IPC_Stream_Perc            = IPC_Stream_Perc2
        uIPC_Stream_Perc           = uIPC_Stream_Perc2
        rawInst_Stream_Perc        = rawInst_Stream_Perc2
	Exe_memkernels_stream      = float(lines_memkernels_stream[8].split()[9])
	MisBr_memkernels_stream    = float(lines_memkernels_stream[25].split()[13])
	IPC_memkernels_stream      = float(lines_memkernels_stream[25].split()[1])
	uIPC_memkernels_stream     = float(lines_memkernels_stream[25].split()[2])
	rawInst_memkernels_stream  = float(lines_memkernels_stream[21].split()[2])

if len(lines_memkernels_stream_omp)==32:
        Exe_Stream_omp_val             = Exe_Stream_omp_val1
        MisBr_Stream_omp_val           = MisBr_Stream_omp_val1
        IPC_Stream_omp_val             = IPC_Stream_omp_val1
        uIPC_Stream_omp_val            = uIPC_Stream_omp_val1
        rawInst_Stream_omp_val         = rawInst_Stream_omp_val1
        Exe_Stream_omp_Perc            = Exe_Stream_omp_Perc1
        MisBr_Stream_omp_Perc          = MisBr_Stream_omp_Perc1
        IPC_Stream_omp_Perc            = IPC_Stream_omp_Perc1
        uIPC_Stream_omp_Perc           = uIPC_Stream_omp_Perc1
        rawInst_Stream_omp_Perc        = rawInst_Stream_omp_Perc1
	Exe_memkernels_stream_omp      = float(lines_memkernels_stream_omp[8].split()[9])
	MisBr_memkernels_stream_omp    = float(lines_memkernels_stream_omp[19].split()[13])
	IPC_memkernels_stream_omp      = float(lines_memkernels_stream_omp[19].split()[1])
	uIPC_memkernels_stream_omp     = float(lines_memkernels_stream_omp[19].split()[2])
	rawInst_memkernels_stream_omp  = float(lines_memkernels_stream_omp[15].split()[2])
elif len(lines_memkernels_stream_omp)==39:
        Exe_Stream_omp_val             = Exe_Stream_omp_val2
        MisBr_Stream_omp_val           = MisBr_Stream_omp_val2
        IPC_Stream_omp_val             = IPC_Stream_omp_val2
        uIPC_Stream_omp_val            = uIPC_Stream_omp_val2
        rawInst_Stream_omp_val         = rawInst_Stream_omp_val2
        Exe_Stream_omp_Perc            = Exe_Stream_omp_Perc2
        MisBr_Stream_omp_Perc          = MisBr_Stream_omp_Perc2
        IPC_Stream_omp_Perc            = IPC_Stream_omp_Perc2
        uIPC_Stream_omp_Perc           = uIPC_Stream_omp_Perc2
        rawInst_Stream_omp_Perc        = rawInst_Stream_omp_Perc2
	Exe_memkernels_stream_omp      = float(lines_memkernels_stream_omp[8].split()[9])
	MisBr_memkernels_stream_omp    = float(lines_memkernels_stream_omp[25].split()[13])
	IPC_memkernels_stream_omp      = float(lines_memkernels_stream_omp[25].split()[1])
	uIPC_memkernels_stream_omp     = float(lines_memkernels_stream_omp[25].split()[2])
	rawInst_memkernels_stream_omp  = float(lines_memkernels_stream_omp[21].split()[2])

if len(lines_launcher_blackscholes)==32: 
        Exe_blackscholes_val             = Exe_blackscholes_val1
        MisBr_blackscholes_val           = MisBr_blackscholes_val1
        IPC_blackscholes_val             = IPC_blackscholes_val1
        uIPC_blackscholes_val            = uIPC_blackscholes_val1
        rawInst_blackscholes_val         = rawInst_blackscholes_val1
        Exe_blackscholes_Perc            = Exe_blackscholes_Perc1
        MisBr_blackscholes_Perc          = MisBr_blackscholes_Perc1
        IPC_blackscholes_Perc            = IPC_blackscholes_Perc1
        uIPC_blackscholes_Perc           = uIPC_blackscholes_Perc1
        rawInst_blackscholes_Perc        = rawInst_blackscholes_Perc1
	Exe_launcher_blackscholes       = float(lines_launcher_blackscholes[8].split()[9])
	MisBr_launcher_blackscholes     = float(lines_launcher_blackscholes[19].split()[13])
	IPC_launcher_blackscholes       = float(lines_launcher_blackscholes[19].split()[1])    
	uIPC_launcher_blackscholes      = float(lines_launcher_blackscholes[19].split()[2])
	rawInst_launcher_blackscholes   = float(lines_launcher_blackscholes[15].split()[2])
elif len(lines_launcher_blackscholes)==39:
        Exe_blackscholes_val             = Exe_blackscholes_val2
        MisBr_blackscholes_val           = MisBr_blackscholes_val2
        IPC_blackscholes_val             = IPC_blackscholes_val2 
        uIPC_blackscholes_val            = uIPC_blackscholes_val2 
        rawInst_blackscholes_val         = rawInst_blackscholes_val2 
        Exe_blackscholes_Perc            = Exe_blackscholes_Perc2
        MisBr_blackscholes_Perc          = MisBr_blackscholes_Perc2
        IPC_blackscholes_Perc            = IPC_blackscholes_Perc2
        uIPC_blackscholes_Perc           = uIPC_blackscholes_Perc2
        rawInst_blackscholes_Perc        = rawInst_blackscholes_Perc2
	Exe_launcher_blackscholes        = float(lines_launcher_blackscholes[8].split()[9])
	MisBr_launcher_blackscholes      = float(lines_launcher_blackscholes[25].split()[13])
	IPC_launcher_blackscholes        = float(lines_launcher_blackscholes[25].split()[1])
	uIPC_launcher_blackscholes       = float(lines_launcher_blackscholes[25].split()[2])
	rawInst_launcher_blackscholes    = float(lines_launcher_blackscholes[21].split()[2])
     
if len(lines_launcher_fft)==32:  
        Exe_fft_val             = Exe_fft_val1
        MisBr_fft_val           = MisBr_fft_val1
        IPC_fft_val             = IPC_fft_val1
        uIPC_fft_val            = uIPC_fft_val1
        rawInst_fft_val         = rawInst_fft_val1
        Exe_fft_Perc            = Exe_fft_Perc1
        MisBr_fft_Perc          = MisBr_fft_Perc1
        IPC_fft_Perc            = IPC_fft_Perc1
        uIPC_fft_Perc           = uIPC_fft_Perc1
        rawInst_fft_Perc        = rawInst_fft_Perc1
	Exe_launcher_fft                = float(lines_launcher_fft[8].split()[9])
	MisBr_launcher_fft              = float(lines_launcher_fft[19].split()[13])
	IPC_launcher_fft                = float(lines_launcher_fft[19].split()[1])
	uIPC_launcher_fft               = float(lines_launcher_fft[19].split()[2])
	rawInst_launcher_fft            = float(lines_launcher_fft[15].split()[2])
elif len(lines_launcher_fft)==39:  
        Exe_fft_val             = Exe_fft_val2
        MisBr_fft_val           = MisBr_fft_val2
        IPC_fft_val             = IPC_fft_val2
        uIPC_fft_val            = uIPC_fft_val2
        rawInst_fft_val         = rawInst_fft_val2
        Exe_fft_Perc            = Exe_fft_Perc2
        MisBr_fft_Perc          = MisBr_fft_Perc2
        IPC_fft_Perc            = IPC_fft_Perc2
        uIPC_fft_Perc           = uIPC_fft_Perc2
        rawInst_fft_Perc        = rawInst_fft_Perc2
	Exe_launcher_fft                 = float(lines_launcher_fft[8].split()[9])
	MisBr_launcher_fft               = float(lines_launcher_fft[25].split()[13])
	IPC_launcher_fft                 = float(lines_launcher_fft[25].split()[1])		
	uIPC_launcher_fft                = float(lines_launcher_fft[25].split()[2])
	rawInst_launcher_fft             = float(lines_launcher_fft[21].split()[2])


output_table_file = open(regression_tables_Dir+"Regression_table.txt","w")
output_table_file.write(" ,        stream,stream_omp,blackscholes,fft \n")
output_table_file.write("Exe,      "+repr(Exe_memkernels_stream)+","+repr(Exe_memkernels_stream_omp)+","+repr(Exe_launcher_blackscholes)+","+repr(Exe_launcher_fft)+","+"\n")
output_table_file.write("MisBr,    "+repr(MisBr_memkernels_stream)+","+repr(MisBr_memkernels_stream_omp)+","+repr(MisBr_launcher_blackscholes)+","+repr(MisBr_launcher_fft)+","+"\n")
output_table_file.write("IPC,      "+repr(IPC_memkernels_stream)+","+repr(IPC_memkernels_stream_omp)+","+repr(IPC_launcher_blackscholes)+","+repr(IPC_launcher_fft)+","+"\n")
output_table_file.write("uIPC,     "+repr(uIPC_memkernels_stream)+","+repr(uIPC_memkernels_stream_omp)+","+repr(uIPC_launcher_blackscholes)+","+repr(uIPC_launcher_fft)+","+"\n")
output_table_file.write("rawInst,  "+repr(rawInst_memkernels_stream)+","+repr(rawInst_memkernels_stream_omp)+","+repr(rawInst_launcher_blackscholes)+","+repr(rawInst_launcher_fft)+","+"\n")
output_table_file.close()

'''
output_table_file = open(regression_tables_Dir+"Regression_table2.txt","w")
output_table_file.write(" ,        stream,stream_omp,blackscholes,fft \n")
output_table_file.write("Exe,      "+repr(Exe_memkernels_stream2)+","+repr(Exe_memkernels_stream_omp2)+","+repr(Exe_launcher_blackscholes2)+","+repr(Exe_launcher_fft2)+","+"\n")
output_table_file.write("MisBr,    "+repr(MisBr_memkernels_stream2)+","+repr(MisBr_memkernels_stream_omp_proc_02)+","+repr(MisBr_launcher_blackscholes2)+","+repr(MisBr_launcher_fft2)+","+"\n")
output_table_file.write("IPC,      "+repr(IPC_memkernels_stream2)+","+repr(IPC_memkernels_stream_omp_proc_02)+","+repr(IPC_launcher_blackscholes2)+","+repr(IPC_launcher_fft2)+","+"\n")
output_table_file.write("uIPC,     "+repr(uIPC_memkernels_stream2)+","+repr(uIPC_memkernels_stream_omp_proc_02)+","+repr(uIPC_launcher_blackscholes2)+","+repr(uIPC_launcher_fft2)+","+"\n")
output_table_file.write("rawInst,  "+repr(rawInst_memkernels_stream2)+","+repr(rawInst_memkernels_stream_omp_proc_02)+","+repr(rawInst_launcher_blackscholes2)+","+repr(rawInst_launcher_fft2)+","+"\n")
output_table_file.close()
'''
	
if (dzer((Exe_memkernels_stream - Exe_Stream_val), Exe_Stream_val) <=  Exe_Stream_Perc):
       	print "Exe in benchmark Stream is ........ OK"
elif (dzer((Exe_memkernels_stream - Exe_Stream_val), Exe_Stream_val) < 1.5 * Exe_Stream_Perc):
        print "Exe in benchmark Stream is ........ slightly above normal "
else:
       	print "Exe in benchmark Stream is ........ way over normal"

if (dzer((MisBr_memkernels_stream - MisBr_Stream_val), MisBr_Stream_val) <=  MisBr_Stream_Perc):
        print "MisBr in benchmark Stream is ........ OK"
elif (dzer((MisBr_memkernels_stream - MisBr_Stream_val), MisBr_Stream_val) < 1.5 * MisBr_Stream_Perc):
       	print "MisBr in benchmark Stream is ........ slightly above normal "
else:
       	print "MisBr in benchmark Stream is ........ way over normal"

if (dzer((IPC_memkernels_stream - IPC_Stream_val), IPC_Stream_val) <=  IPC_Stream_Perc):
       	print "IPC in benchmark Stream is ........ OK"
elif (dzer((IPC_memkernels_stream - IPC_Stream_val), IPC_Stream_val) < 1.5 * IPC_Stream_Perc):
       	print "IPC in benchmark Stream is ........ slightly above normal "
else:
       	print "IPC in benchmark Stream is ........ way over normal"

if (dzer((uIPC_memkernels_stream - uIPC_Stream_val), uIPC_Stream_val) <=  uIPC_Stream_Perc):
       	print "uIPC in benchmark Stream is ........ OK"
elif (dzer((uIPC_memkernels_stream - uIPC_Stream_val), uIPC_Stream_val) < 1.5 * uIPC_Stream_Perc):
       	print "uIPC in benchmark Stream is ........ slightly above normal "
else:
       	print "uIPC in benchmark Stream is ........ way over normal"

if (dzer((rawInst_memkernels_stream - rawInst_Stream_val), rawInst_Stream_val) <=  rawInst_Stream_Perc):
       	print "rawInst in benchmark Stream is ........ OK"
elif (dzer((rawInst_memkernels_stream - rawInst_Stream_val), rawInst_Stream_val) < 1.5 * rawInst_Stream_Perc):
       	print "rawInst in benchmark Stream is ........ slightly above normal "
else:
       	print "rawInst in benchmark Stream is ........ way over normal"

if (dzer((Exe_memkernels_stream_omp - Exe_Stream_omp_val), Exe_Stream_omp_val) <=  Exe_Stream_omp_Perc):
       	print "Exe in benchmark Stream_omp is ........ OK"
elif (dzer((Exe_memkernels_stream_omp - Exe_Stream_omp_val), Exe_Stream_omp_val) < 1.5 * Exe_Stream_omp_Perc):
       	print "Exe in benchmark Stream_omp is ........ slightly above normal "
else:
       	print "Exe in benchmark Stream_omp is ........ way over normal"

if (dzer((MisBr_memkernels_stream_omp - MisBr_Stream_omp_val), MisBr_Stream_omp_val) <=  MisBr_Stream_omp_Perc):
       	print "MisBr in benchmark Stream_omp is ........ OK"
elif (dzer((MisBr_memkernels_stream_omp - MisBr_Stream_omp_val), MisBr_Stream_omp_val) < 1.5 * MisBr_Stream_omp_Perc):
       	print "MisBr in benchmark Stream_omp is ........ slightly above normal "
else:
       	print "MisBr in benchmark Stream_omp is ........ way over normal"

if (dzer((IPC_memkernels_stream_omp - IPC_Stream_omp_val), IPC_Stream_omp_val) <=  IPC_Stream_omp_Perc):
       	print "IPC in benchmark Stream_omp is ........ OK"
elif (dzer((IPC_memkernels_stream_omp - IPC_Stream_omp_val), IPC_Stream_omp_val) < 1.5 * IPC_Stream_omp_Perc):
       	print "IPC in benchmark Stream_omp is ........ slightly above normal "
else:
       	print "IPC in benchmark Stream_omp is ........ way over normal"

if (dzer((uIPC_memkernels_stream_omp - uIPC_Stream_omp_val), uIPC_Stream_omp_val) <=  uIPC_Stream_omp_Perc):
       	print "uIPC in benchmark Stream_omp is ........ OK"
elif (dzer((uIPC_memkernels_omp_stream_omp - uIPC_Stream_omp_val), uIPC_Stream_omp_val) < 1.5 * uIPC_Stream_omp_Perc):
       	print "uIPC in benchmark Stream_omp is ........ slightly above normal "
else:
       	print "uIPC in benchmark Stream_omp is ........ way over normal"

if (dzer((rawInst_memkernels_stream_omp - rawInst_Stream_omp_val), rawInst_Stream_omp_val) <=  rawInst_Stream_omp_Perc):
       	print "rawInst in benchmark Stream_omp is ........ OK"
elif (dzer((rawInst_memkernels_stream_omp - rawInst_Stream_omp_val), rawInst_Stream_omp_val) < 1.5 * rawInst_Stream_omp_Perc):
    	print "rawInst in benchmark Stream_omp is ........ slightly above normal "
else:
       	print "rawInst in benchmark Stream_omp is ........ way over normal"

if (dzer((Exe_launcher_blackscholes - Exe_blackscholes_val), Exe_blackscholes_val) <=  Exe_blackscholes_Perc):
       	print "Exe in benchmark blackscholes is ........ OK"
elif (dzer((Exe_launcher_blackscholes - Exe_blackscholes_val), Exe_blackscholes_val) < 1.5 * Exe_blackscholes_Perc):
       	print "Exe in benchmark blackscholes is ........ slightly above normal "
else:
       	print "Exe in benchmark blackscholes is ........ way over normal"

if (dzer((MisBr_launcher_blackscholes - MisBr_blackscholes_val), MisBr_blackscholes_val) <=  MisBr_blackscholes_Perc):
        print "MisBr in benchmark blackscholes is ........ OK"
elif (dzer((MisBr_launcher_blackscholes - MisBr_blackscholes_val), MisBr_blackscholes_val) < 1.5 * MisBr_blackscholes_Perc):
        print "MisBr in benchmark blackscholes is ........ slightly above normal "
else:	
	print "MisBr in benchmark blackscholes is ........ way over normal"

if (dzer((IPC_launcher_blackscholes - IPC_blackscholes_val), IPC_blackscholes_val) <=  IPC_blackscholes_Perc):
        print "IPC in benchmark blackscholes is ........ OK"
elif (dzer((IPC_launcher_blackscholes - IPC_blackscholes_val), IPC_blackscholes_val) < 1.5 * IPC_blackscholes_Perc):
        print "IPC in benchmark blackscholes is ........ slightly above normal "
else:
        print "IPC in benchmark blackscholes is ........ way over normal"
	
if (dzer((uIPC_launcher_blackscholes - uIPC_blackscholes_val), uIPC_blackscholes_val) <=  uIPC_blackscholes_Perc):
        print "uIPC in benchmark blackscholes is ........ OK"
elif (dzer((uIPC_launcher_blackscholes - uIPC_blackscholes_val), uIPC_blackscholes_val) < 1.5 * uIPC_blackscholes_Perc):
        print "uIPC in benchmark blackscholes is ........ slightly above normal "
else:
        print "uIPC in benchmark blackscholes is ........ way over normal"

if (dzer((rawInst_launcher_blackscholes - rawInst_blackscholes_val), rawInst_blackscholes_val) <=  rawInst_blackscholes_Perc):
        print "rawInst in benchmark blackscholes is ........ OK"
elif (dzer((rawInst_launcher_blackscholes - rawInst_blackscholes_val), rawInst_blackscholes_val) < 1.5 * rawInst_blackscholes_Perc):
        print "rawInst in benchmark blackscholes is ........ slightly above normal "
else:
        print "rawInst in benchmark blackscholes is ........ way over normal"
	
if (dzer((Exe_launcher_fft - Exe_fft_val), Exe_fft_val) <=  Exe_fft_Perc):
        print "Exe in benchmark fft is ........ OK"
elif (dzer((Exe_launcher_fft - Exe_fft_val), Exe_fft_val) < 1.5 * Exe_fft_Perc):
        print "Exe in benchmark fft is ........ slightly above normal "
else:
        print "Exe in benchmark fft is ........ way over normal"

if (dzer((MisBr_launcher_fft - MisBr_fft_val), MisBr_fft_val) <=  MisBr_fft_Perc):
        print "MisBr in benchmark fft is ........ OK"
elif (dzer((MisBr_launcher_fft - MisBr_fft_val), MisBr_fft_val) < 1.5 * MisBr_fft_Perc):
        print "MisBr in benchmark fft is ........ slightly above normal "
else:
        print "MisBr in benchmark fft is ........ way over normal"
	
if (dzer((IPC_launcher_fft - IPC_fft_val), IPC_fft_val) <=  IPC_fft_Perc):
        print "IPC in benchmark fft is ........ OK"
elif (dzer((IPC_launcher_fft - IPC_fft_val), IPC_fft_val) < 1.5 * IPC_fft_Perc):
        print "IPC in benchmark fft is ........ slightly above normal "
else:
        print "IPC in benchmark fft is ........ way over normal"
	
if (dzer((uIPC_launcher_fft - uIPC_fft_val), uIPC_fft_val) <=  uIPC_fft_Perc):
       	print "uIPC in benchmark fft is ........ OK"
elif (dzer((uIPC_launcher_fft - uIPC_fft_val), uIPC_fft_val) < 1.5 * uIPC_fft_Perc):
        print "uIPC in benchmark fft is ........ slightly above normal "
else:
        print "uIPC in benchmark fft is ........ way over normal"
	
if (dzer((rawInst_launcher_fft - rawInst_fft_val), rawInst_fft_val) <=  rawInst_fft_Perc):
        print "rawInst in benchmark fft is ........ OK"
elif (dzer((rawInst_launcher_fft - rawInst_fft_val), rawInst_fft_val) < 1.5 * rawInst_fft_Perc):
        print "rawInst in benchmark fft is ........ slightly above normal "
else:
        print "rawInst in benchmark fft is ........ way over normal"

ASD = (dzer((Exe_memkernels_stream - Exe_Stream_val), Exe_Stream_val) <=  Exe_Stream_Perc) +\
      (dzer((MisBr_memkernels_stream - MisBr_Stream_val), MisBr_Stream_val) <=  MisBr_Stream_Perc) +\
      (dzer((IPC_memkernels_stream - IPC_Stream_val), IPC_Stream_val) <=  IPC_Stream_Perc)+\
      (dzer((uIPC_memkernels_stream - uIPC_Stream_val), uIPC_Stream_val) <=  uIPC_Stream_Perc) +\
      (dzer((rawInst_memkernels_stream - rawInst_Stream_val), rawInst_Stream_val) <=  rawInst_Stream_Perc) +\
      (dzer((Exe_memkernels_stream_omp - Exe_Stream_omp_val), Exe_Stream_omp_val) <=  Exe_Stream_omp_Perc)+\
      (dzer((MisBr_memkernels_stream_omp - MisBr_Stream_omp_val), MisBr_Stream_omp_val) <=  MisBr_Stream_omp_Perc)+\
      (dzer((IPC_memkernels_stream_omp - IPC_Stream_omp_val), IPC_Stream_omp_val) <=  IPC_Stream_omp_Perc)+\
      (dzer((uIPC_memkernels_stream_omp - uIPC_Stream_omp_val), uIPC_Stream_omp_val) <=  uIPC_Stream_omp_Perc)+\
      (dzer((rawInst_memkernels_stream_omp - rawInst_Stream_omp_val), rawInst_Stream_omp_val) <= rawInst_Stream_omp_Perc)+\
      (dzer((Exe_launcher_blackscholes - Exe_blackscholes_val), Exe_blackscholes_val) <=  Exe_blackscholes_Perc)+\
      (dzer((MisBr_launcher_blackscholes - MisBr_blackscholes_val), MisBr_blackscholes_val) <=  MisBr_blackscholes_Perc)+\
      (dzer((IPC_launcher_blackscholes - IPC_blackscholes_val), IPC_blackscholes_val) <=  IPC_blackscholes_Perc)+\
      (dzer((uIPC_launcher_blackscholes - uIPC_blackscholes_val), uIPC_blackscholes_val) <=  uIPC_blackscholes_Perc)+\
      (dzer((rawInst_launcher_blackscholes - rawInst_blackscholes_val), rawInst_blackscholes_val) <=  rawInst_blackscholes_Perc)+\
      (dzer((Exe_launcher_fft - Exe_fft_val), Exe_fft_val) <=  Exe_fft_Perc)+\
      (dzer((MisBr_launcher_fft - MisBr_fft_val), MisBr_fft_val) <=  MisBr_fft_Perc)+\
      (dzer((IPC_launcher_fft - IPC_fft_val), IPC_fft_val) <=  IPC_fft_Perc)+\
      (dzer((uIPC_launcher_fft - uIPC_fft_val), uIPC_fft_val) <=  uIPC_fft_Perc)+\
      (dzer((rawInst_launcher_fft - rawInst_fft_val), rawInst_fft_val) <=  rawInst_fft_Perc)

if (ASD==20):  print "Regression test successfully passed  ............... All OK"
else: print "Could not pass regression test"
