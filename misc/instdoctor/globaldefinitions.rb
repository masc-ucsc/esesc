############################################################################
#    Copyright (C) 2010 by Alamelu Sankaranarayanan   #
#    alamelu@cse.ucsc.edu   #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################
#These sets have been compiled from the PTX v2.0 manual.

require 'set'

$PTX_SESC_TRANS_CSV = "PTX2SESC.csv"

$PTX_DIRECTIVES = Set.new [".align" , ".func" , ".minnctapersm" , ".reg" , ".tex" , ".const" , ".global" , ".maxnreg" , ".section" , ".version" , ".entry" , ".local" , ".maxntid" , ".shared" , ".visible" , ".extern" , ".loc" , ".param" , ".sreg" , ".file" , ".maxnctapersm" , ".pragma" , ".target"]

$PTX_CONST_TEX = Set.new [".tex" , ".const"]

$PTX_RESERVED_INSTRUCTION_KEYWORDS = Set.new ["abs" , "div" , "or" , "slct" , "vshl add" , "ex2" , "pmevent" , "sqrt" , "vshr addc" , "exit" , "popc" , "st" , "vsub","and" , "fma" , "prefetch" , "sub" , "vote atom" , "isspacep" , "prefetchu" , "subc bar" , "ld" , "prmt" , "suld","bfe" , "ldu" , "rcp" , "sured" , "bfi" , "lg2" , "red" , "sust", "bfind" , "mad" , "rem" , "suq", "bra" , "mad24" , "ret" , "tex"," brev" , "max" , "rsqrt" , "txq", "brkpt" , "membar" , "sad" , "trap","call" , "min" , "selp" , "vabsdiff", "clz" , "mov" , "set" , "vadd", "cnot" , "mul" , "setp" , "vmad", "cos" , "mul24" , "shl" , "vmax", "cvt" , "neg" , "shr" , "vmin", "cvta" , "not" , "sin" , "vset", "xor"]

$PTX_PREDEFINED_IDENTIFIERS = Set.new ["%tid" , "%ctaid" , "%laneid" , "%laneid_lt" , "%clock" , "%ntid" , "%nctaaid" , "%gridid" , "%laneid_ge" , "%clock64" , "%warpid" , "%smid" , "%lanemask_eq" , "%laneid_gt" , "%pm0
%pm1", "%pm2","%pm3" , "%nwarpid" , "%nsmid" , "%lanemask_le" , "WARP_SZ"]

$PTX_STATE_SPACES = Set.new [".reg",".sreg",".const",".global",".local",".param",".shared",".tex"]

$PTX_INTEGER_ARITHMETIC_INST = Set.new ["add", "sub", "add.cc", "addc", "sub.cc", "subc", "mul", "mad", "mul24", "mad24", "sad", "div", "rem", "abs", "neg", "min", "max", "popc", "clz", "bfind", "brev", "bfe", "bfi", "prmt"]

$PTX_FLOATING_POINT_INST = Set.new ["testp", "copysign", "add", "sub", "mul", "fma", "mad", "div", "abs", "neg", "min", "max", "rcp", "sqrt", "rsqrt", "sin", "cos", "lg2", "ex2"]

$PTX_COMPARISON_SELECTION_INST = Set.new ["set", "setp", "selp", "slct"]

$PTX_LOGIC_SHIFT_INST = Set.new ["and", "or", "xor", "not", "cnot", "shl", "shr"]

$PTX_DATA_MOVT_CONV_INST = Set.new ["mov", "ld", "ldu", "st", "prefetch", "prefetchu", "isspacep", "cvta", "cvt"]

$PTX_TEXTURE_SURFACE_INST = Set.new ["tex", "txq", "suld", "sust", "sured", "suq"]

$PTX_CONTROL_FLOW_INST = Set.new ["{", "}", "@", "bra", "call", "ret", "exit"]

$PTX_SYNC_COMM_INST = Set.new ["bar", "membar", "atom", "red", "vote"]

$PTX_VIDEO_INST = Set.new ["vadd", "vsub", "vabsdiff", "vmin", "vmax", "vshl", "vshr", "vmad", "vset"]

$PTX_MISC_INST = Set.new ["trap", "brkpt", "pmevent"]

$PTX_SPECIAL_REGISTERS = Set.new ["%tid", "%ntid", "%laneid", "%warpid", "%nwarpid", "%ctaid", "%nctaid", "%smid", "%nsmid", "%gridid", "	%lanemask_eq", "%lanemask_le", "%lanemask_lt", "%lanemask_ge", "%lanemask_gt", "%clock", "%clock64", "%pm0","%pm1","%pm2","%pm3"]

$PTX_VER_TARGET_DIRECTIVES = Set.new [".version", ".target"]

$PTX_KERNELFUNC_DIRECTIVES = Set.new [".entry", ".func"]

$PTX_PERF_TUNING_DIRECTIVES = Set.new [".maxnreg", ".maxntid",	".maxnctapersm", ".minnctapersm", ".pragma"]

$PTX_DEBUGGING_DIRECTIVES = Set.new ["@@DWARF", ".section", ".file", ".loc"]

$PTX_LINKING_DIRECTIVES = Set.new [".extern", ".visible"]

$PTX_LEADER_INSTRUCTIONS = "@|bar|bra|call|ret"



#Global Variables
#---------------------------------------------------------------------#
# $PTX_hashmap = Hash.new        #Key: Line Number Value: Object: PTXLineStruct
$PTX_hashmap = Array.new
$Kernel_array = Array.new



#These global variables are initialized when a new function/kernel is initialized
#---------------------------------------------------------------------#
$bb = Hash.new                 # KEY: BBID, VALUE: BB
$instDecode = Hash.new         # KEY: LINE NUMBER, VALUE: (DECODED) INSTRUCTION
$addressmap = Hash.new         # KEY: register, Value: Position in the respective array
$shmemmap = Hash.new           # Key: Address, Value: Instructions to store the value into Shmem pointer
$labels = Hash.new
$P2S = Hash.new                # Key: ptx opcode, Value: translated esesc instruction
$registers = Hash.new					 # Key: register, Value: type
$programconstants = Hash.new		# Key: variable, Value: type
$predicates = Hash.new					#Key predicate, Value: bbtp offset

# Global Constants : To be read from the configuration file (bbtrace.h as of now).
#---------------------------------------------------------------------#
DEBUG = false
#$PC = 1000      #Starts at this address, will change in the course of the program
$PC = 1073741824  #0x40000000      #Starts at this address, will change in the course of the program
$MAXTRACESIZE = 40
$MAXINSTPERBB = 100

if ($M32)
	$BBTPOFFSET = 40
else
	$BBTPOFFSET = 80
end

# NOTE : If you end adding or subtracting a field : look for "trc_ptr_size"
#        and BBTPOFFSET. Also you will need to change the print statements
#        that are printed after the contamination (look for " is the size of the
#        the trace pointer". Look for these three terms in kernel.rb)
#        You also need to change the tracesize that is printed to the *info file
#        (Look for "TRACESIZE=")
#[0] Bits 0-7  ---> nextbb, if its 0, then go to the terminal block
#[1] Bits 8-15  ---> currentBBID
#[2] Bits 16-23  ---> skipinstcount
#[3] Bits 24-31  ---> branchtaken-nottaken,
#[4] Bits 32-39  ---> pause
#[5] Bits 40-47  ---> unified BlockID
#[6] Bits 48-55  ---> uinfied threadID
#[7] Bits 56-63  ---> smid
#[8] Bits 64-71  ---> warpid
#[9] Bits 72-79  ---> instructions fast forwarded;
#[10] Bits 80-87  ---> Branch count;
#[11] Bits 88-95  ---> Total Taken branches; 
#[12-xx] --> locations to store the predicate (xx is determined and varies from benchmark to benchmark)



#In case32 bit compilation is enabled :
#Bits 0-3  ---> nextbb, if its 0, then go to the terminal block
#Bits 4-7  ---> currentBBID
#Bits 8-11  ---> skipinstcount
#Bits 12-15  ---> branchtaken-nottaken,
#Bits 16-19  ---> pause
#Bits 20-23  ---> unified BlockID
#Bits 24-27  ---> uinfied threadID
#Bits 28-31  ---> smid
#Bits 32-35  ---> warpid
#Bits 36-39  ---> instructions fast forwarded;
#[10] 40-43  ---> Taken branch count;
#[11] 44-47  ---> Total branches; 
#[12-xx] --> locations to store the predicate (xx is determined and varies from benchmark to benchmark)



# Default values, will be assigned later
$DFL_REG32 = 64 # FIXME : Can be made local to the kernel class
$DFL_REG64 = 64 # FIXME : Can be made local to the kernel class
$DFL_REGFP = 64 # FIXME : Can be made local to the kernel class
$DFL_REGFP64 = 64 # FIXME : Can be made local to the kernel class
$TRACESIZE = 40 # FIXME : Can be made local to the kernel class

$DFL_SM32 = 16 # FIXME : Can be made local to the kernel class
$DFSM64 = 16 # FIXME : Can be made local to the kernel class
$DFL_SMFP = 16 # FIXME : Can be made local to the kernel class
$DFL_SMFP64 = 16 # FIXME : Can be made local to the kernel class
$DFL_SMADDR =48 #(16+16+16) # FIXME : Can be made local to the kernel class

#---------------------------------------------------------------------#
#$BLOCKSIZE = 16 # Should match the value used when compiling the files. ***************** OUTDATED ****************
#
#$BLOCKSIZEX = 16 # Should match the value used when compiling the files.
#$BLOCKSIZEY = 16 # Should match the value used when compiling the files.
#$BLOCKSIZEZ = 1 # Should match the value used when compiling the files.
$M32 = false	# FLAG to set 32 bit compilation
#---------------------------------------------------------------------#

# Default values, assigned when the contaminated ptx is read, assigned in instruction.rb
#---------------------------------------------------------------------#
$TRACECTXPTR_32 = ""
$TRACECTXPTR_64 = ""
$TRACECTXPTR_FP = ""
$TRACECTXPTR_FP64 = ""
$SHMPTR_32 = ""
$SHMPTR_64 = ""
$SHMPTR_FP = ""
$SHMPTR_FP64 = ""
$SHMPTR_ADDR = ""


$FIRSTBB = 1                                   #Set  to specify where the basic block count should begin from. Strictly > 0
$lastlocation = $BBTPOFFSET                    #points To The Location In The Tracepointer Where The Last Mem Ref Was Stored.
$lastLabel = String.new

#Used for Live Variable Analysis only
#---------------------------------------------------------------------#
$USE = Array.new
$DEF = Array.new
$LIVEIN = Array.new
$LIVEOUT = Array.new
$LIVEINDASH = Array.new
$LIVEOUTDASH = Array.new
$LINENO = Array.new


# Used to track the usage of the context arrays
#---------------------------------------------------------------------#
$BBCP32_pos = 0
$BBCPFP_pos = 0
$BBCPFP64_pos = 0
$BBCP64_pos = 0

$SHM32_pos = 0 #32 bit data
$SHMFP_pos = 0 #floating point data
$SHMFP64_pos = 0 #floating point data
$SHM64_pos = 0 #64 bit data
$SHM_addr= 0 #64 bit destination address

#Global Structures
PTXLineStruct = Struct.new("PTXLine", :line_number, :line_type, :line)
PTXParamType = Struct.new("ParamType", :paramname, :paramwidth)
PTX_sesc_Translation = Struct.new("PTXTrans", :esesc_opcode, :jumplabel, :decompose)
