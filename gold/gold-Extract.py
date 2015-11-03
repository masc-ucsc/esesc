from __future__ import division
import math
import datetime
import sys

build_Addr = sys.argv[1]
proj_Addr  = sys.argv[2]

report_Dir = build_Addr+"/gold/report/"
tables_Dir  = build_Addr+"/gold/tables/"

Exe_Stream_List       = []
Exe_Stream_omp_List   = []
Exe_blackscholes_List = []
Exe_fft_List          = [] 

MisBr_Stream_List       = []
MisBr_Stream_omp_List   = []
MisBr_blackscholes_List = []
MisBr_fft_List          = []

IPC_Stream_List       = []
IPC_Stream_omp_List   = []
IPC_blackscholes_List = []
IPC_fft_List          = []

uIPC_Stream_List       = []
uIPC_Stream_omp_List   = []
uIPC_blackscholes_List = []
uIPC_fft_List          = []

rawInst_Stream_List       = []
rawInst_Stream_omp_List   = []
rawInst_blackscholes_List = []
rawInst_fft_List          = []  

Exe_Stream_List2       = []
Exe_Stream_omp_List2   = []
Exe_blackscholes_List2 = []
Exe_fft_List2          = []

MisBr_Stream_List2       = []
MisBr_Stream_omp_List2   = []
MisBr_blackscholes_List2 = []
MisBr_fft_List2          = []

IPC_Stream_List2       = []
IPC_Stream_omp_List2   = []
IPC_blackscholes_List2 = []
IPC_fft_List2          = []

uIPC_Stream_List2       = []
uIPC_Stream_omp_List2   = []
uIPC_blackscholes_List2 = []
uIPC_fft_List2          = []

rawInst_Stream_List2       = []
rawInst_Stream_omp_List2   = []
rawInst_blackscholes_List2 = []
rawInst_fft_List2          = []

def trunc(f):
    slen = len('%.*f' % (5, f))
    return str(f)[:slen]

def trunc3(f):
    slen = len('%.*f' % (3, f))
    return str(f)[:slen]

def dzer(a,b):
    if b==0:
       return 0
    else: 
       return a/b 

def mmax(mlist):
    if len(mlist) > 0: 
	sssd = max(mlist)
    else: 
        sssd = 0
    return sssd 

def AVG(my_list):
    sum_of_grades = sum(my_list)
    average = dzer(sum_of_grades, len(my_list))
    return average

for counter in range(10):
 
	file_memkernels_stream  = open (report_Dir+"report_memkernels_stream_"+`counter+1`+".txt", "r") 
	lines_memkernels_stream = file_memkernels_stream.readlines()
	file_memkernels_stream.close()
	
	file_memkernels_stream_omp = open (report_Dir+"report_memkernels_stream_omp_"+`counter+1`+".txt", "r")
	lines_memkernels_stream_omp = file_memkernels_stream_omp.readlines()
	file_memkernels_stream_omp.close()
	
	file_launcher_blackscholes=open(report_Dir+"report_launcher_blackscholes_"+`counter+1`+".txt","r")
	lines_launcher_blackscholes=file_launcher_blackscholes.readlines() 
	file_launcher_blackscholes.close()

	file_launcher_fft = open (report_Dir+"report_launcher_fft_"+`counter+1`+".txt","r") 
	lines_launcher_fft = file_launcher_fft.readlines()
	file_launcher_fft.close()	

        if len(lines_memkernels_stream)==32: 
		 Exe_memkernels_stream     = lines_memkernels_stream[8].split()[9]
		 Exe_Stream_List.append(Exe_memkernels_stream)
		 MisBr_memkernels_stream   = lines_memkernels_stream[19].split()[13]
		 MisBr_Stream_List.append(MisBr_memkernels_stream)	 
		 IPC_memkernels_stream     = lines_memkernels_stream[19].split()[1]
		 IPC_Stream_List.append(IPC_memkernels_stream)
	         uIPC_memkernels_stream    = lines_memkernels_stream[19].split()[2]
		 uIPC_Stream_List.append(uIPC_memkernels_stream)
	         rawInst_memkernels_stream = lines_memkernels_stream[15].split()[2]
		 rawInst_Stream_List.append(rawInst_memkernels_stream)
        elif len(lines_memkernels_stream)==39:
                 Exe_memkernels_stream     = lines_memkernels_stream[8].split()[9]
                 Exe_Stream_List2.append(Exe_memkernels_stream)
                 MisBr_memkernels_stream   = lines_memkernels_stream[25].split()[13]
                 MisBr_Stream_List2.append(MisBr_memkernels_stream)
                 IPC_memkernels_stream     = lines_memkernels_stream[25].split()[1]
                 IPC_Stream_List2.append(IPC_memkernels_stream)
                 uIPC_memkernels_stream    = lines_memkernels_stream[25].split()[2]
                 uIPC_Stream_List2.append(uIPC_memkernels_stream)
                 rawInst_memkernels_stream = lines_memkernels_stream[21].split()[2]
                 rawInst_Stream_List2.append(rawInst_memkernels_stream)

        if len(lines_memkernels_stream_omp)==32:
                Exe_memkernels_stream_omp               = lines_memkernels_stream_omp[8].split()[9]
                Exe_Stream_omp_List.append(Exe_memkernels_stream_omp)
                MisBr_memkernels_stream_omp_proc_0      = lines_memkernels_stream_omp[19].split()[13]
                MisBr_Stream_omp_List.append(MisBr_memkernels_stream_omp_proc_0)
                IPC_memkernels_stream_omp_proc_0        = lines_memkernels_stream_omp[19].split()[1]
                IPC_Stream_omp_List.append(IPC_memkernels_stream_omp_proc_0)
                uIPC_memkernels_stream_omp_proc_0       = lines_memkernels_stream_omp[19].split()[2]
                uIPC_Stream_omp_List.append(uIPC_memkernels_stream_omp_proc_0)
                rawInst_memkernels_stream_omp_proc_0    = lines_memkernels_stream_omp[15].split()[2]
                rawInst_Stream_omp_List.append(rawInst_memkernels_stream_omp_proc_0)
        elif len(lines_memkernels_stream_omp)==39:
                Exe_memkernels_stream_omp               = lines_memkernels_stream_omp[8].split()[9]
                Exe_Stream_omp_List2.append(Exe_memkernels_stream_omp)
                MisBr_memkernels_stream_omp_proc_0      = lines_memkernels_stream_omp[25].split()[13]
                MisBr_Stream_omp_List2.append(MisBr_memkernels_stream_omp_proc_0)
                IPC_memkernels_stream_omp_proc_0        = lines_memkernels_stream_omp[25].split()[1]
                IPC_Stream_omp_List2.append(IPC_memkernels_stream_omp_proc_0)
                uIPC_memkernels_stream_omp_proc_0       = lines_memkernels_stream_omp[25].split()[2]
                uIPC_Stream_omp_List2.append(uIPC_memkernels_stream_omp_proc_0)
		rawInst_memkernels_stream_omp_proc_0    = lines_memkernels_stream_omp[21].split()[2]
		rawInst_Stream_omp_List2.append(rawInst_memkernels_stream_omp_proc_0)

        if len(lines_launcher_blackscholes)==32: 
	        Exe_launcher_blackscholes       = lines_launcher_blackscholes[8].split()[9]
		Exe_blackscholes_List.append(Exe_launcher_blackscholes)
   	        MisBr_launcher_blackscholes     = lines_launcher_blackscholes[19].split()[13]
		MisBr_blackscholes_List.append(MisBr_launcher_blackscholes)
      	        IPC_launcher_blackscholes       = lines_launcher_blackscholes[19].split()[1]    
		IPC_blackscholes_List.append(IPC_launcher_blackscholes)
	        uIPC_launcher_blackscholes      = lines_launcher_blackscholes[19].split()[2]
                uIPC_blackscholes_List.append(uIPC_launcher_blackscholes)
	        rawInst_launcher_blackscholes   = lines_launcher_blackscholes[15].split()[2]
                rawInst_blackscholes_List.append(rawInst_launcher_blackscholes)
 	elif len(lines_launcher_blackscholes)==39:
                Exe_launcher_blackscholes       = lines_launcher_blackscholes[8].split()[9]
                Exe_blackscholes_List2.append(Exe_launcher_blackscholes)
                MisBr_launcher_blackscholes     = lines_launcher_blackscholes[25].split()[13]
                MisBr_blackscholes_List2.append(MisBr_launcher_blackscholes)
                IPC_launcher_blackscholes       = lines_launcher_blackscholes[25].split()[1]
                IPC_blackscholes_List2.append(IPC_launcher_blackscholes)
                uIPC_launcher_blackscholes      = lines_launcher_blackscholes[25].split()[2]
                uIPC_blackscholes_List2.append(uIPC_launcher_blackscholes)
                rawInst_launcher_blackscholes   = lines_launcher_blackscholes[21].split()[2]
                rawInst_blackscholes_List2.append(rawInst_launcher_blackscholes)
             
        if len(lines_launcher_fft)==32:  
                Exe_launcher_fft                = lines_launcher_fft[8].split()[9]
                Exe_fft_List.append(Exe_launcher_fft)
	        MisBr_launcher_fft              = lines_launcher_fft[19].split()[13]
                MisBr_fft_List.append(MisBr_launcher_fft)
	        IPC_launcher_fft                = lines_launcher_fft[19].split()[1]
                IPC_fft_List.append(IPC_launcher_fft)
	        uIPC_launcher_fft               = lines_launcher_fft[19].split()[2]
                uIPC_fft_List.append(uIPC_launcher_fft)
		rawInst_launcher_fft            = lines_launcher_fft[15].split()[2]
                rawInst_fft_List.append(rawInst_launcher_fft)
        elif len(lines_launcher_fft)==39:  
                Exe_launcher_fft                = lines_launcher_fft[8].split()[9]
                Exe_fft_List2.append(Exe_launcher_fft)
	        MisBr_launcher_fft              = lines_launcher_fft[25].split()[13]
                MisBr_fft_List2.append(MisBr_launcher_fft)
	        IPC_launcher_fft                = lines_launcher_fft[25].split()[1]
                IPC_fft_List2.append(IPC_launcher_fft)
	        uIPC_launcher_fft               = lines_launcher_fft[25].split()[2]
                uIPC_fft_List2.append(uIPC_launcher_fft)
		rawInst_launcher_fft            = lines_launcher_fft[21].split()[2]
                rawInst_fft_List2.append(rawInst_launcher_fft)
'''
print "stream 32 size is: "+str(len(Exe_Stream_List))
print "stream 39 size is: "+str(len(Exe_Stream_List2))
print "stream_omp 32 size is: "+str(len(Exe_Stream_omp_List))
print "stream_omp 39 size is: "+str(len(Exe_Stream_omp_List2))
print "blackscholes 32 size is: "+str(len(Exe_blackscholes_List))
print "blackscholes 39 size is: "+str(len(Exe_blackscholes_List2))
print "fft 32 size is: "+str(len(Exe_fft_List))
print "fft 39 size is: "+str(len(Exe_fft_List2))

print MisBr_Stream_List
'''
Exe_Stream_List = map(float, Exe_Stream_List)
Exe_Stream_Avg  = AVG(Exe_Stream_List)
Exe_Stream_Max  = mmax(Exe_Stream_List)
Exe_Stream_Dev  = trunc(dzer(math.fabs(Exe_Stream_Max-Exe_Stream_Avg), math.fabs(Exe_Stream_Avg))*100)

MisBr_Stream_List = map(float, MisBr_Stream_List)
MisBr_Stream_Avg  = AVG(MisBr_Stream_List)
MisBr_Stream_Max  = mmax(MisBr_Stream_List)
MisBr_Stream_Dev  = trunc(dzer(math.fabs(MisBr_Stream_Max-MisBr_Stream_Avg), math.fabs(MisBr_Stream_Avg))*100)

IPC_Stream_List = map(float, IPC_Stream_List)
IPC_Stream_Avg  = AVG(IPC_Stream_List)
IPC_Stream_Max  = mmax(IPC_Stream_List)
IPC_Stream_Dev  = trunc(dzer(math.fabs(IPC_Stream_Max-IPC_Stream_Avg),math.fabs(IPC_Stream_Avg))*100)

uIPC_Stream_List = map(float, uIPC_Stream_List)
uIPC_Stream_Avg  = AVG(uIPC_Stream_List)
uIPC_Stream_Max  = mmax(uIPC_Stream_List)
uIPC_Stream_Dev  = trunc(dzer(math.fabs(uIPC_Stream_Max-uIPC_Stream_Avg),math.fabs(uIPC_Stream_Avg))*100)

rawInst_Stream_List = map(float, rawInst_Stream_List)
rawInst_Stream_Avg  = AVG(rawInst_Stream_List)
rawInst_Stream_Max  = mmax(rawInst_Stream_List)
rawInst_Stream_Dev  = trunc(dzer(math.fabs(rawInst_Stream_Max-rawInst_Stream_Avg),math.fabs(rawInst_Stream_Avg))*100)

Exe_Stream_omp_List = map(float, Exe_Stream_omp_List)
Exe_Stream_omp_Avg  = AVG(Exe_Stream_omp_List)
Exe_Stream_omp_Max  = mmax(Exe_Stream_omp_List)
Exe_Stream_omp_Dev  = trunc(dzer(math.fabs(Exe_Stream_omp_Max-Exe_Stream_omp_Avg),math.fabs(Exe_Stream_omp_Avg))*100)

MisBr_Stream_omp_List = map(float, MisBr_Stream_omp_List)
MisBr_Stream_omp_Avg  = AVG(MisBr_Stream_omp_List)
MisBr_Stream_omp_Max  = mmax(MisBr_Stream_omp_List)
MisBr_Stream_omp_Dev  = trunc(dzer(math.fabs(MisBr_Stream_omp_Max-MisBr_Stream_omp_Avg),math.fabs(MisBr_Stream_omp_Avg))*100)

IPC_Stream_omp_List = map(float, IPC_Stream_omp_List)
IPC_Stream_omp_Avg  = AVG(IPC_Stream_omp_List)
IPC_Stream_omp_Max  = mmax(IPC_Stream_omp_List)
IPC_Stream_omp_Dev  = trunc(dzer(math.fabs(IPC_Stream_omp_Max-IPC_Stream_omp_Avg),math.fabs(IPC_Stream_omp_Avg))*100)

uIPC_Stream_omp_List = map(float, uIPC_Stream_omp_List)
uIPC_Stream_omp_Avg  = AVG(uIPC_Stream_omp_List)
uIPC_Stream_omp_Max  = mmax(uIPC_Stream_omp_List)
uIPC_Stream_omp_Dev  = trunc(dzer(math.fabs(uIPC_Stream_omp_Max-uIPC_Stream_omp_Avg),math.fabs(uIPC_Stream_omp_Avg))*100)

rawInst_Stream_omp_List = map(float, rawInst_Stream_omp_List)
rawInst_Stream_omp_Avg  = AVG(rawInst_Stream_omp_List)
rawInst_Stream_omp_Max  = mmax(rawInst_Stream_omp_List)
rawInst_Stream_omp_Dev  = trunc(dzer(math.fabs(rawInst_Stream_omp_Max-rawInst_Stream_omp_Avg),math.fabs(rawInst_Stream_omp_Avg))*100)

Exe_blackscholes_List = map(float, Exe_blackscholes_List)
Exe_blackscholes_Avg  = AVG(Exe_blackscholes_List)
Exe_blackscholes_Max  = mmax(Exe_blackscholes_List)
Exe_blackscholes_Dev  = trunc(dzer(math.fabs(Exe_blackscholes_Max-Exe_blackscholes_Avg),math.fabs(Exe_blackscholes_Avg))*100)

MisBr_blackscholes_List = map(float, MisBr_blackscholes_List)
MisBr_blackscholes_Avg  = AVG(MisBr_blackscholes_List)
MisBr_blackscholes_Max  = mmax(MisBr_blackscholes_List)
MisBr_blackscholes_Dev  = trunc(dzer(math.fabs(MisBr_blackscholes_Max-MisBr_blackscholes_Avg),math.fabs(MisBr_blackscholes_Avg))*100)

IPC_blackscholes_List = map(float, IPC_blackscholes_List)
IPC_blackscholes_Avg  = AVG(IPC_blackscholes_List)
IPC_blackscholes_Max  = mmax(IPC_blackscholes_List)
IPC_blackscholes_Dev  = trunc(dzer(math.fabs(IPC_blackscholes_Max-IPC_blackscholes_Avg),math.fabs(IPC_blackscholes_Avg))*100)

uIPC_blackscholes_List = map(float, uIPC_blackscholes_List)
uIPC_blackscholes_Avg  = AVG(uIPC_blackscholes_List)
uIPC_blackscholes_Max  = mmax(uIPC_blackscholes_List)
uIPC_blackscholes_Dev  = trunc(dzer(math.fabs((uIPC_blackscholes_Max-uIPC_blackscholes_Avg)),math.fabs(uIPC_blackscholes_Avg))*100)

rawInst_blackscholes_List = map(float, rawInst_blackscholes_List)
rawInst_blackscholes_Avg  = AVG(rawInst_blackscholes_List)
rawInst_blackscholes_Max  = mmax(rawInst_blackscholes_List)
rawInst_blackscholes_Dev  = trunc(dzer(math.fabs((rawInst_blackscholes_Max-rawInst_blackscholes_Avg)),math.fabs(rawInst_blackscholes_Avg))*100)

Exe_fft_List = map(float, Exe_fft_List)
Exe_fft_Avg  = AVG(Exe_fft_List)
Exe_fft_Max  = mmax(Exe_fft_List)
Exe_fft_Dev  = trunc(dzer(math.fabs((Exe_fft_Max-Exe_fft_Avg)),math.fabs(Exe_fft_Avg))*100)

MisBr_fft_List = map(float, MisBr_fft_List)
MisBr_fft_Avg  = AVG(MisBr_fft_List)
MisBr_fft_Max  = mmax(MisBr_fft_List)
MisBr_fft_Dev  = trunc(dzer(math.fabs((MisBr_fft_Max-MisBr_fft_Avg)),math.fabs(MisBr_fft_Avg))*100)

IPC_fft_List = map(float, IPC_fft_List)
IPC_fft_Avg  = AVG(IPC_fft_List)
IPC_fft_Max  = mmax(IPC_fft_List)
IPC_fft_Dev  = trunc(dzer(math.fabs((IPC_fft_Max-IPC_fft_Avg)),math.fabs(IPC_fft_Avg))*100)

uIPC_fft_List = map(float, uIPC_fft_List)
uIPC_fft_Avg  = AVG(uIPC_fft_List)
uIPC_fft_Max  = mmax(uIPC_fft_List)
uIPC_fft_Dev  = trunc(dzer(math.fabs((uIPC_fft_Max-uIPC_fft_Avg)),math.fabs(uIPC_fft_Avg))*100)

rawInst_fft_List = map(float, rawInst_fft_List)
rawInst_fft_Avg  = AVG(rawInst_fft_List)
rawInst_fft_Max  = mmax(rawInst_fft_List)
rawInst_fft_Dev  = trunc(dzer(math.fabs((rawInst_fft_Max-rawInst_fft_Avg)),math.fabs(rawInst_fft_Avg))*100)
#-----------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------
Exe_Stream_List2 = map(float, Exe_Stream_List2)
Exe_Stream_Avg2  = AVG(Exe_Stream_List2)
Exe_Stream_Max2  = mmax(Exe_Stream_List2)
Exe_Stream_Dev2  = trunc(dzer(math.fabs(Exe_Stream_Max2-Exe_Stream_Avg2), math.fabs(Exe_Stream_Avg2))*100)

MisBr_Stream_List2 = map(float, MisBr_Stream_List2)
MisBr_Stream_Avg2  = AVG(MisBr_Stream_List2)
MisBr_Stream_Max2  = mmax(MisBr_Stream_List2)
MisBr_Stream_Dev2  = trunc(dzer(math.fabs(MisBr_Stream_Max2-MisBr_Stream_Avg2), math.fabs(MisBr_Stream_Avg2))*100)

IPC_Stream_List2 = map(float, IPC_Stream_List2)
IPC_Stream_Avg2  = AVG(IPC_Stream_List2)
IPC_Stream_Max2  = mmax(IPC_Stream_List2)
IPC_Stream_Dev2  = trunc(dzer(math.fabs(IPC_Stream_Max2-IPC_Stream_Avg2),math.fabs(IPC_Stream_Avg2))*100)

uIPC_Stream_List2 = map(float, uIPC_Stream_List2)
uIPC_Stream_Avg2  = AVG(uIPC_Stream_List2)
uIPC_Stream_Max2  = mmax(uIPC_Stream_List2)
uIPC_Stream_Dev2  = trunc(dzer(math.fabs(uIPC_Stream_Max2-uIPC_Stream_Avg2),math.fabs(uIPC_Stream_Avg2))*100)

rawInst_Stream_List2 = map(float, rawInst_Stream_List2)
rawInst_Stream_Avg2  = AVG(rawInst_Stream_List2)
rawInst_Stream_Max2  = mmax(rawInst_Stream_List2)
rawInst_Stream_Dev2  = trunc(dzer(math.fabs(rawInst_Stream_Max2-rawInst_Stream_Avg2),math.fabs(rawInst_Stream_Avg2))*100)

Exe_Stream_omp_List2 = map(float, Exe_Stream_omp_List2)
Exe_Stream_omp_Avg2  = AVG(Exe_Stream_omp_List2)
Exe_Stream_omp_Max2  = mmax(Exe_Stream_omp_List2)
Exe_Stream_omp_Dev2  = trunc(dzer(math.fabs(Exe_Stream_omp_Max2-Exe_Stream_omp_Avg2),math.fabs(Exe_Stream_omp_Avg2))*100)

MisBr_Stream_omp_List2 = map(float, MisBr_Stream_omp_List2)
MisBr_Stream_omp_Avg2  = AVG(MisBr_Stream_omp_List2)
MisBr_Stream_omp_Max2  = mmax(MisBr_Stream_omp_List2)
MisBr_Stream_omp_Dev2  = trunc(dzer(math.fabs(MisBr_Stream_omp_Max2-MisBr_Stream_omp_Avg2),math.fabs(MisBr_Stream_omp_Avg2))*100)

IPC_Stream_omp_List2 = map(float, IPC_Stream_omp_List2)
IPC_Stream_omp_Avg2  = AVG(IPC_Stream_omp_List2)
IPC_Stream_omp_Max2  = mmax(IPC_Stream_omp_List2)
IPC_Stream_omp_Dev2  = trunc(dzer(math.fabs(IPC_Stream_omp_Max2-IPC_Stream_omp_Avg2),math.fabs(IPC_Stream_omp_Avg2))*100)

uIPC_Stream_omp_List2 = map(float, uIPC_Stream_omp_List2)
uIPC_Stream_omp_Avg2  = AVG(uIPC_Stream_omp_List2)
uIPC_Stream_omp_Max2  = mmax(uIPC_Stream_omp_List2)
uIPC_Stream_omp_Dev2  = trunc(dzer(math.fabs(uIPC_Stream_omp_Max2-uIPC_Stream_omp_Avg2),math.fabs(uIPC_Stream_omp_Avg2))*100)

rawInst_Stream_omp_List2 = map(float, rawInst_Stream_omp_List2)
rawInst_Stream_omp_Avg2  = AVG(rawInst_Stream_omp_List2)
rawInst_Stream_omp_Max2  = mmax(rawInst_Stream_omp_List2)
rawInst_Stream_omp_Dev2  = trunc(dzer(math.fabs(rawInst_Stream_omp_Max2-rawInst_Stream_omp_Avg2),math.fabs(rawInst_Stream_omp_Avg2))*100)

Exe_blackscholes_List2 = map(float, Exe_blackscholes_List2)
Exe_blackscholes_Avg2  = AVG(Exe_blackscholes_List2)
Exe_blackscholes_Max2  = mmax(Exe_blackscholes_List2)
Exe_blackscholes_Dev2  = trunc(dzer(math.fabs(Exe_blackscholes_Max2-Exe_blackscholes_Avg2),math.fabs(Exe_blackscholes_Avg2))*100)

MisBr_blackscholes_List2 = map(float, MisBr_blackscholes_List2)
MisBr_blackscholes_Avg2  = AVG(MisBr_blackscholes_List2)
MisBr_blackscholes_Max2  = mmax(MisBr_blackscholes_List2)
MisBr_blackscholes_Dev2  = trunc(dzer(math.fabs(MisBr_blackscholes_Max2-MisBr_blackscholes_Avg2),math.fabs(MisBr_blackscholes_Avg2))*100)

IPC_blackscholes_List2 = map(float, IPC_blackscholes_List2)
IPC_blackscholes_Avg2  = AVG(IPC_blackscholes_List2)
IPC_blackscholes_Max2  = mmax(IPC_blackscholes_List2)
IPC_blackscholes_Dev2  = trunc(dzer(math.fabs(IPC_blackscholes_Max2-IPC_blackscholes_Avg2),math.fabs(IPC_blackscholes_Avg2))*100)

uIPC_blackscholes_List2 = map(float, uIPC_blackscholes_List2)
uIPC_blackscholes_Avg2  = AVG(uIPC_blackscholes_List2)
uIPC_blackscholes_Max2  = mmax(uIPC_blackscholes_List2)
uIPC_blackscholes_Dev2  = trunc(dzer(math.fabs((uIPC_blackscholes_Max2-uIPC_blackscholes_Avg2)),math.fabs(uIPC_blackscholes_Avg2))*100)

rawInst_blackscholes_List2 = map(float, rawInst_blackscholes_List2)
rawInst_blackscholes_Avg2  = AVG(rawInst_blackscholes_List2)
rawInst_blackscholes_Max2  = mmax(rawInst_blackscholes_List2)
rawInst_blackscholes_Dev2  = trunc(dzer(math.fabs((rawInst_blackscholes_Max2-rawInst_blackscholes_Avg2)),math.fabs(rawInst_blackscholes_Avg2))*100)

Exe_fft_List2 = map(float, Exe_fft_List2)
Exe_fft_Avg2  = AVG(Exe_fft_List2)
Exe_fft_Max2  = mmax(Exe_fft_List2)
Exe_fft_Dev2  = trunc(dzer(math.fabs((Exe_fft_Max2-Exe_fft_Avg2)),math.fabs(Exe_fft_Avg2))*100)

MisBr_fft_List2 = map(float, MisBr_fft_List2)
MisBr_fft_Avg2  = AVG(MisBr_fft_List2)
MisBr_fft_Max2  = mmax(MisBr_fft_List2)
MisBr_fft_Dev2  = trunc(dzer(math.fabs((MisBr_fft_Max2-MisBr_fft_Avg2)),math.fabs(MisBr_fft_Avg2))*100)

IPC_fft_List2 = map(float, IPC_fft_List2)
IPC_fft_Avg2  = AVG(IPC_fft_List2)
IPC_fft_Max2  = mmax(IPC_fft_List2)
IPC_fft_Dev2  = trunc(dzer(math.fabs((IPC_fft_Max2-IPC_fft_Avg2)),math.fabs(IPC_fft_Avg2))*100)

uIPC_fft_List2 = map(float, uIPC_fft_List2)
uIPC_fft_Avg2  = AVG(uIPC_fft_List2)
uIPC_fft_Max2  = mmax(uIPC_fft_List2)
uIPC_fft_Dev2  = trunc(dzer(math.fabs((uIPC_fft_Max2-uIPC_fft_Avg2)),math.fabs(uIPC_fft_Avg2))*100)

rawInst_fft_List2 = map(float, rawInst_fft_List2)
rawInst_fft_Avg2  = AVG(rawInst_fft_List2)
rawInst_fft_Max2  = mmax(rawInst_fft_List2)
rawInst_fft_Dev2  = trunc(dzer(math.fabs((rawInst_fft_Max2-rawInst_fft_Avg2)),math.fabs(rawInst_fft_Avg2))*100)
#---------------------------------------------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------------------------------------------
output_table_file = open(tables_Dir+"Max_Dev_Percentage_table1.txt","w")
output_table_file.write(" ,        stream,stream_omp,blackscholes,fft \n")
output_table_file.write("Exe,      "+repr(Exe_Stream_Dev)+","+repr(Exe_Stream_omp_Dev)+","+repr(Exe_blackscholes_Dev)+","+repr(Exe_fft_Dev)+","+"\n") 
output_table_file.write("MisBr,    "+repr(MisBr_Stream_Dev)+","+repr(MisBr_Stream_omp_Dev) +","+repr(MisBr_blackscholes_Dev)+","+repr(MisBr_fft_Dev)+","+"\n")
output_table_file.write("IPC,      "+repr(IPC_Stream_Dev)+","+repr(IPC_Stream_omp_Dev) +","+repr(IPC_blackscholes_Dev)+","+repr(IPC_fft_Dev)+","+"\n")
output_table_file.write("uIPC,     "+repr(uIPC_Stream_Dev)+","+repr(uIPC_Stream_omp_Dev) +","+repr(uIPC_blackscholes_Dev)+","+repr(uIPC_fft_Dev)+","+"\n")
output_table_file.write("rawInst,  "+repr(rawInst_Stream_Dev)+","+repr(rawInst_Stream_omp_Dev) +","+repr(rawInst_blackscholes_Dev)+","+repr(rawInst_fft_Dev)+","+"\n")
output_table_file.close()

output_table_file = open(tables_Dir+"Gold_Avg_Value_table1.txt","w")
output_table_file.write(" ,        stream,stream_omp,blackscholes,fft \n")
output_table_file.write("Exe,      "+repr(trunc3(Exe_Stream_Avg))+","+repr(trunc3(Exe_Stream_omp_Avg))+","+repr(trunc3(Exe_blackscholes_Avg))+","+repr(trunc3(Exe_fft_Avg))+","+"\n")
output_table_file.write("MisBr,    "+repr(trunc3(MisBr_Stream_Avg))+","+repr(trunc3(MisBr_Stream_omp_Avg)) +","+repr(trunc3(MisBr_blackscholes_Avg))+","+repr(trunc3(MisBr_fft_Avg))+","+"\n")
output_table_file.write("IPC,      "+repr(trunc3(IPC_Stream_Avg))+","+repr(trunc3(IPC_Stream_omp_Avg)) +","+repr(trunc3(IPC_blackscholes_Avg))+","+repr(trunc3(IPC_fft_Avg))+","+"\n")
output_table_file.write("uIPC,     "+repr(trunc3(uIPC_Stream_Avg))+","+repr(trunc3(uIPC_Stream_omp_Avg)) +","+repr(trunc3(uIPC_blackscholes_Avg))+","+repr(trunc3(uIPC_fft_Avg))+","+"\n")
output_table_file.write("rawInst,  "+repr(trunc3(rawInst_Stream_Avg))+","+repr(trunc3(rawInst_Stream_omp_Avg)) +","+repr(trunc3(rawInst_blackscholes_Avg))+","+repr(trunc3(rawInst_fft_Avg))+","+"\n")
output_table_file.close()
#---------------------------------------------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------------------------------------------
output_table_file = open(tables_Dir+"Max_Dev_Percentage_table2.txt","w")
output_table_file.write(" ,        stream,stream_omp,blackscholes,fft \n")
output_table_file.write("Exe,      "+repr(Exe_Stream_Dev2)+","+repr(Exe_Stream_omp_Dev2)+","+repr(Exe_blackscholes_Dev2)+","+repr(Exe_fft_Dev2)+","+"\n")
output_table_file.write("MisBr,    "+repr(MisBr_Stream_Dev2)+","+repr(MisBr_Stream_omp_Dev2) +","+repr(MisBr_blackscholes_Dev2)+","+repr(MisBr_fft_Dev2)+","+"\n")
output_table_file.write("IPC,      "+repr(IPC_Stream_Dev2)+","+repr(IPC_Stream_omp_Dev2) +","+repr(IPC_blackscholes_Dev2)+","+repr(IPC_fft_Dev2)+","+"\n")
output_table_file.write("uIPC,     "+repr(uIPC_Stream_Dev2)+","+repr(uIPC_Stream_omp_Dev2) +","+repr(uIPC_blackscholes_Dev2)+","+repr(uIPC_fft_Dev2)+","+"\n")
output_table_file.write("rawInst,  "+repr(rawInst_Stream_Dev2)+","+repr(rawInst_Stream_omp_Dev2) +","+repr(rawInst_blackscholes_Dev2)+","+repr(rawInst_fft_Dev2)+","+"\n")
output_table_file.close()

output_table_file = open(tables_Dir+"Gold_Avg_Value_table2.txt","w")
output_table_file.write(" ,        stream,stream_omp,blackscholes,fft \n") 
output_table_file.write("Exe,      "+repr(trunc3(Exe_Stream_Avg2))+","+repr(trunc3(Exe_Stream_omp_Avg2))+","+repr(trunc3(Exe_blackscholes_Avg2))+","+repr(trunc3(Exe_fft_Avg2))+","+"\n")
output_table_file.write("MisBr,    "+repr(trunc3(MisBr_Stream_Avg2))+","+repr(trunc3(MisBr_Stream_omp_Avg2)) +","+repr(trunc3(MisBr_blackscholes_Avg2))+","+repr(trunc3(MisBr_fft_Avg2))+","+"\n")
output_table_file.write("IPC,      "+repr(trunc3(IPC_Stream_Avg2))+","+repr(trunc3(IPC_Stream_omp_Avg2)) +","+repr(trunc3(IPC_blackscholes_Avg2))+","+repr(trunc3(IPC_fft_Avg2))+","+"\n")
output_table_file.write("uIPC,     "+repr(trunc3(uIPC_Stream_Avg2))+","+repr(trunc3(uIPC_Stream_omp_Avg2)) +","+repr(trunc3(uIPC_blackscholes_Avg2))+","+repr(trunc3(uIPC_fft_Avg2))+","+"\n")
output_table_file.write("rawInst,  "+repr(trunc3(rawInst_Stream_Avg2))+","+repr(trunc3(rawInst_Stream_omp_Avg2)) +","+repr(trunc3(rawInst_blackscholes_Avg2))+","+repr(trunc3(rawInst_fft_Avg2))+","+"\n")
output_table_file.close()

