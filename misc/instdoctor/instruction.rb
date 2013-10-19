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

require 'set'

class Instruction

  # Components of the Instruction
  @label
  @opcode
  @pred
  @dest
  @destOff
  @desta  #For setp Instruction that writes two instructions.
  @src1
  @src1Off
  @src2
  @src2Off
  @src3
  @src3Off
  @offset
  @immSrc1
  @immSrc2
  @immSrc3

  # Attributes of the Instruction
  @leader
  @line_number
  @ignore

  # Needed for printing the Instruction
  @dumpString

  #Needed for Live Variable Analysis
  @defSet
  @useSet
  @liveInSet
  @liveOutSet

  #Needed by memory instructions to denote position in the array of context pointers for sh and gl memory
  @arraypos
  @saveSharedState
  @is_shmload

  #-----------------------------------------------------------------------------------------------------------------------#
  def isImmSrc1
    return @immSrc1
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def isImmSrc2
    return @immSrc2
  end

  def isImmSrc3
    return @immSrc3
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def getInst
    return @dumpString
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def getPred
    return @pred
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def getDest
    return @dest
  end

  def getDestOff
    return @destOff
  end
  def getSrc1
    return @src1
  end
  def getSrc1Off
    return @src1Off
  end
  def getSrc2
    return @src2
  end
  def getSrc2Off
    return @src2Off
  end
  def getSrc3
    return @src3
  end
  def getSrc3Off
    return @src3Off
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def getLabel
    return @label
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def getOpcode
    return @opcode
  end
  #-----------------------------------------------------------------------------------------------------------------------#

  def getUseSet
    return @useSet
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def getDefSet
    return @defSet
  end
  #-----------------------------------------------------------------------------------------------------------------------#

  def isInst
    return !@ignore
  end

  def setAsLeader
    @leader = true
  end
  #-----------------------------------------------------------------------------------------------------------------------#

  def setLiveIns(ip_set)
    @liveInSet = ip_set
  end
  #-----------------------------------------------------------------------------------------------------------------------#

  def setLiveOuts(op_set)
    @liveInSet = op_set
  end
  #-----------------------------------------------------------------------------------------------------------------------#

  #-----------------------------------------------------------------------------------------------------------------------#
  def getSharedStoreCode
    return @saveSharedState
  end

  def isLeader
    return @leader
  end
  #-----------------------------------------------------------------------------------------------------------------------#

  def initialize(line)
    @dumpString = line
    @ignore = true

    @immSrc1 = false
    @immSrc2 = false
    @immSrc3 = false

    tempflag = false
    line = line.split(';')[0]

    #puts line
    lineType = whatType(line)

    if(lineType == "ExtractParam")
      par = line.split(' ')[2]
      if /d_trace/.match(par)
        $TRACEPTR = par
        $TRACEPTR.delete! ","
        $TRACEPTR.delete! "\)"
      elsif /d_context_32/.match(par)
        $TRACECTXPTR_32 = (par)
        $TRACECTXPTR_32.delete! ","
        $TRACECTXPTR_32.delete! "\)"
      elsif /d_context_64/.match(par)
        $TRACECTXPTR_64 = (par)
        $TRACECTXPTR_64.delete! ","
        $TRACECTXPTR_64.delete! "\)"
      elsif /d_context_fp64/.match(par)
        $TRACECTXPTR_FP64 = (par)
        $TRACECTXPTR_FP64.delete! ","
        $TRACECTXPTR_FP64.delete! "\)"
      elsif /d_context_fp/.match(par)
        $TRACECTXPTR_FP = (par)
        $TRACECTXPTR_FP.delete! ","
        $TRACECTXPTR_FP.delete! "\)"
      elsif /d_shared_32/.match(par)
        $SHMPTR_32 = (par)
        $SHMPTR_32.delete! ","
        $SHMPTR_32.delete! "\)"
      elsif /d_shared_64/.match(par)
        $SHMPTR_64 = (par)
        $SHMPTR_64.delete! ","
        $SHMPTR_64.delete! "\)"
      elsif /d_shared_fp64/.match(par)
        $SHMPTR_FP64 = (par)
        $SHMPTR_FP64.delete! ","
        $SHMPTR_FP64.delete! "\)"
      elsif /d_shared_fp/.match(par)
        $SHMPTR_FP = (par)
        $SHMPTR_FP.delete! ","
        $SHMPTR_FP.delete! "\)"
      elsif /d_shared_addr/.match(par)
        $SHMPTR_ADDR = (par)
        $SHMPTR_ADDR.delete! ","
        $SHMPTR_ADDR.delete! "\)"
      end

      #elsif(lineType == "Directive")
      #  processsRegisterStateSpaceDirective(line)

    elsif(lineType == "Label(next line)")
      @label = line.split(':')[0]
      # whether or not this instruction is a leader will be determined later.

    elsif(lineType == "Label(this line)")
      #Separate label from line, and process rest of the instruction.
      # whether or not this instruction is a leader will be determined later.
      @label = line.split(':')[0]
      @ignore = false
      processInstruction(linesplit(':')[0]) #TODO


    elsif(lineType == "Predicated Instruction")
      @ignore = false
      #Separate predicate from line...
      temp = line.split(' ')
      line = ""
      temp.each do |item|
        if  /^\s*(@)/.match(item)
          @pred = item
        else
          line.concat(item)
          line.concat(" ")
        end
      end

      line.strip!
      # ... and process rest of the instruction.
      processInstruction(line)
      if /bra|bar|call|exit/.match(@opcode)
        tempflag = false
      else
        tempflag = true
      end


    elsif(lineType == "Alignment directive")
      #Separate alignment directive from line, and process rest of the instruction.
      processAlignedInstruction(line)#TODO

    elsif lineType == "Instruction"
      @ignore = false

      processInstruction(line)
      if /bra|bar|call|exit/.match(@opcode)
        tempflag = false
      else
        tempflag = true
        # 								if /global/.match(@opcode)
        # 									if /ld|st/.match(@opcode)
        # 										if /u32|s32/.match(@opcode)
        # 											@arraypos = $BBCP32_pos
        # 											$BBCP32_pos += 1
        # 										elsif /u64|s64/.match(@opcode)
        # 											@arraypos = $BBCP64_pos
        # 											$BBCP64_pos += 1
        # 										elsif /f32/.match(@opcode)
        # 											@arraypos = $BBCPFP_pos
        # 											$BBCPFP_pos += 1
        # 										end
        # 									end

        if /setp|pred/.match(@opcode)
          #puts "SETPSETP #{@opcode} at #{line} Predicate : #{getDest()} @%#{getDest()} st.global.u32 [bbtp+0],1"
          if !($predicates.has_key?("#{getDest()}"))
            $predicates["#{getDest()}"]=$BBTPOFFSET;
            if ($M32)
              $BBTPOFFSET=$BBTPOFFSET+4;
            else
              $BBTPOFFSET=$BBTPOFFSET+8;
            end
          end
        end



        if /shared/.match(@opcode)

          if /st/.match(@opcode)
            if /u32|s32/.match(@opcode)
              @arraypos = $SHM32_pos

              @saveSharedState = ""
              @saveSharedState += "//Save ShMem State\n"

              if (getSrc1Off)
                if /__cuda/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u32 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u64 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                elsif /d/.match(getSrc1()) then
                  if ($M32)
                    puts "Here 277"
                    puts "Instruction.rb: Is this an error? flag m32 is set, and there are 64 bit variables in use??"
                    exit(-1);
                    @saveSharedState += "\tadd.u32         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tadd.u64         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                else
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u32 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u64 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end

                end
              else
                if /__cuda/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                elsif /d/.match(getSrc1()) then
                  if ($M32)
                    puts "Here 301"
                    puts @opcode
                    puts getSrc1()
                    puts "Instruction.rb: Is this an error? flag m32 is set, and there are 64 bit variables in use??"
                    exit(-1);
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %#{getSrc1()};\n"
                  else
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %#{getSrc1()};\n"
                  end
                else
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end

                end
              end

              @saveSharedState += "\tst.global.u32   [%smcp32+#{$SHM32_pos*4}], %#{getSrc2};\n"
              #@saveSharedState += "\tst.global.u32   [%smcp32+#{$SHM32_pos*4}], %#{getDest};\n"
              @saveSharedState += "// *********\n"
              if (DEBUG)
                puts @saveSharedState
              end

              str = ""
              if ($M32)
                str += "\tld.global.u32 %smaddr, [%smcpaddr+#{$SHM_addr*4}];\n"
                str += "\tld.global.u32 %sm32data, [%smcp32+#{$SHM32_pos*4}];\n"
                str += "\tst.shared.u32   [%smaddr], %sm32data;\n"
              else
                str += "\tld.global.u64 %smaddr, [%smcpaddr+#{$SHM_addr*8}];\n"
                str += "\tld.global.u32 %sm32data, [%smcp32+#{$SHM32_pos*4}];\n"
                str += "\tst.shared.u32   [%smaddr], %sm32data;\n"
              end

              if (DEBUG)
                puts str
              end
              #                       str += "st.shared.u32 [%smaddr+#{$SHM_addr*8}], [%smcp32+#{$SHM32_pos*4}] "

              $shmemmap["#{$SHM_addr}"] = str
              $SHM_addr += 1
              $SHM32_pos += 1

            elsif /u64|s64/.match(@opcode)
              @arraypos = $SHM64_pos
              @saveSharedState = ""
              @saveSharedState += "//Save ShMem State\n"
              # 											if (getSrc1Off)
              # 											 @saveSharedState += "\tadd.u64         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
              # 											 @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
              # 											else
              # 											 @saveSharedState += "\tst.global,u64   [%smcpaddr+#{$SHM_addr*8}], %#{getSrc1()};\n"
              #                     	end

              if (getSrc1Off)
                if /__cuda/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u32 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u64 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                elsif /d/.match(getSrc1()) then
                  if ($M32)
                    puts "Here 362"
                    puts "Instruction.rb: Is this an error? flag m32 is set, and there are 64 bit variables in use??"
                    @saveSharedState += "\tadd.u32         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                    exit(1);
                  else
                    @saveSharedState += "\tadd.u64         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end

                else
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u32 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u64 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                end

              else

                if /__cuda/.match(getSrc1()) then

                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end

                elsif /d/.match(getSrc1()) then
                  if ($M32)
                    puts "Here 387"
                    puts "Instruction.rb: Is this an error? flag m32 is set, and there are 64 bit variables in use??"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %#{getSrc1()};\n"
                    exit(-1);
                  else
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %#{getSrc1()};\n"
                  end

                else
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end

                end
              end

              if ($M32)
                @saveSharedState += "\tst.global.u32   [%smcpfp+#{$SHM64_pos*4}], %#{getSrc2};\n"
                #@saveSharedState += "\tst.global.u32   [%smcpfp+#{$SHM64_pos*4}], %#{getDest};\n"
              else
                @saveSharedState += "\tst.global.u64   [%smcpfp+#{$SHM64_pos*8}], %#{getSrc2};\n"
                #@saveSharedState += "\tst.global.u64   [%smcpfp+#{$SHM64_pos*8}], %#{getDest};\n"
              end

              @saveSharedState += "// *********\n"
              if (DEBUG)
                puts @saveSharedState
              end

              if ($M32)
                str = ""
                str += "\tld.global.u32 %smaddr, [%smcpaddr+#{$SHM_addr*4}];\n"
                str += "\tld.global.u32 %sm64data, [%smcp64+#{$SHM64_pos*4}];\n"
                str += "\tst.shared.u32   [%smaddr], %sm64data;\n"
              else
                str = ""
                str += "\tld.global.u64 %smaddr, [%smcpaddr+#{$SHM_addr*8}];\n"
                str += "\tld.global.u64 %sm64data, [%smcp64+#{$SHM64_pos*8}];\n"
                str += "\tst.shared.u64   [%smaddr], %sm64data;\n"
              end

              if (DEBUG)
                puts str
              end

              #                       str += "st.shared.u64 [%smaddr+#{$SHM_addr*8}], [%smcp64+#{$SHM32_pos*8}] "

              $shmemmap["#{$SHM_addr}"] = str


              $SHM_addr += 1
              $SHM64_pos += 1

            elsif /f64/.match(@opcode)
              @arraypos = $SHMFP64_pos
              @saveSharedState = ""
              @saveSharedState += "//Save ShMem State\n"

              if (getSrc1Off)
                if /__cuda/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u32 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u64 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end

                elsif /d/.match(getSrc1()) then
                  if ($M32)
                    puts "Instruction.rb: Is this an error? flag m32 is set, and there are 64 bit variables in use??"
                    @saveSharedState += "\tadd.u32         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                    exit(-1)
                  else
                    @saveSharedState += "\tadd.u64         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                else
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u32 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u64 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                end
              else
                if /__cuda/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                elsif /d/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %#{getSrc1()};\n"
                  else
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %#{getSrc1()};\n"
                  end
                else
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                end
              end

              #@saveSharedState += "\tst.global.f64   [%smcpfp64+#{$SHMFP64_pos*8}], %#{getDest};\n"
              @saveSharedState += "\tst.global.f64   [%smcpfp64+#{$SHMFP64_pos*8}], %#{getSrc2};\n"
              @saveSharedState += "// *********\n"
              if (DEBUG)
                puts @saveSharedState
              end

              str = ""
              if ($M32)
                str += "\tld.global.u32 %smaddr, [%smcpaddr+#{$SHM_addr*4}];\n"
                str += "\tld.global.f64 %smfp64data, [%smcpfp64+#{$SHMFP_pos*8}];\n"
                str += "\tst.shared.f64   [%smaddr], %smfp64data;\n"
              else
                str += "\tld.global.u64 %smaddr, [%smcpaddr+#{$SHM_addr*4}];\n"
                str += "\tld.global.f64 %smfp64data, [%smcpfp64+#{$SHMFP_pos*8}];\n"
                str += "\tst.shared.f32   [%smaddr], %smfp64data;\n"

              end

              if (DEBUG)
                puts str
              end

              #                       str += "st.shared.f32 [%smaddr+#{$SHM_addr*8}], [%smcpfp+#{$SHMFP_pos*4}] "

              $shmemmap["#{$SHM_addr}"] = str


              $SHM_addr += 1
              $SHMFP64_pos += 1

            elsif /f32/.match(@opcode)
              @arraypos = $SHMFP_pos
              @saveSharedState = ""
              @saveSharedState += "//Save ShMem State\n"
              # 											if (getSrc1Off)
              # 											 @saveSharedState += "\tadd.u64         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
              # 											 @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
              # 											else
              # 											 @saveSharedState += "\tst.global,u64   [%smcpaddr+#{$SHM_addr*8}], %#{getSrc1()};\n"
              #                     	end


              if (getSrc1Off)
                if /__cuda/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u32 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u64 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end

                elsif /d/.match(getSrc1()) then
                  if ($M32)
                    puts "Here 459"
                    puts "Instruction.rb: Is this an error? flag m32 is set, and there are 64 bit variables in use??"
                    @saveSharedState += "\tadd.u32         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                    exit(-1)
                  else
                    @saveSharedState += "\tadd.u64         %bbaux64, %#{getSrc1()}, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                else
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u32 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tadd.u64 %bbaux64, %bbaux64, #{getSrc1Off()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                end
              else
                if /__cuda/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                elsif /d/.match(getSrc1()) then
                  if ($M32)
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %#{getSrc1()};\n"
                  else
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %#{getSrc1()};\n"
                  end
                else
                  if ($M32)
                    @saveSharedState += "\tcvt.u32.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u32   [%smcpaddr+#{$SHM_addr*4}], %bbaux64;\n"
                  else
                    @saveSharedState += "\tcvt.u64.u32      %bbaux64, %#{getSrc1()};\n"
                    @saveSharedState += "\tst.global.u64   [%smcpaddr+#{$SHM_addr*8}], %bbaux64;\n"
                  end
                end
              end


              @saveSharedState += "\tst.global.f32   [%smcpfp+#{$SHMFP_pos*4}], %#{getSrc2};\n"
              #@saveSharedState += "\tst.global.f32   [%smcpfp+#{$SHMFP_pos*4}], %#{getDest};\n"
              @saveSharedState += "// *********\n"
              if (DEBUG)
                puts @saveSharedState
              end

              str = ""
              if ($M32)
                str += "\tld.global.u32 %smaddr, [%smcpaddr+#{$SHM_addr*4}];\n"
                str += "\tld.global.f32 %smfpdata, [%smcpfp+#{$SHMFP_pos*4}];\n"
                str += "\tst.shared.f32   [%smaddr], %smfpdata;\n"
              else
                str += "\tld.global.u64 %smaddr, [%smcpaddr+#{$SHM_addr*8}];\n"
                str += "\tld.global.f32 %smfpdata, [%smcpfp+#{$SHMFP_pos*4}];\n"
                str += "\tst.shared.f32   [%smaddr], %smfpdata;\n"

              end

              if (DEBUG)
                puts str
              end

              #                       str += "st.shared.f32 [%smaddr+#{$SHM_addr*8}], [%smcpfp+#{$SHMFP_pos*4}] "

              $shmemmap["#{$SHM_addr}"] = str


              $SHM_addr += 1
              $SHMFP_pos += 1
            end
          elsif /ld/.match(@opcode)
            @is_shmload = true
          end
        end
      end

    elsif lineType == "Blank"
      @dumpString="\n"

    elsif lineType == "Loop Begining/ Loop terminating"
      setAsLeader()
    end

    if (tempflag)
      @defSet = Set.new()
      if !(/__cuda/.match(@dest)) && !(/(t|cta)id/.match(@dest)) && !(/setp/.match(@opcode))

          @defSet.add(@dest)
      end

      if !(/__cuda/.match(@desta)) && !(/(t|cta)id/.match(@desta)) && !(/setp/.match(@opcode))
        @defSet.add(@desta)
      end

      @defSet.delete(nil)
      @useSet = Set.new()
      if (!isImmSrc1()) then
        if !(/^\d/.match(@src1) ) && !(/__cuda/.match(@src1)) && !(/(t|cta)id/.match(@src1))
          if !(/p/.match(@src1))
            @useSet.add(@src1)
          end
        end
      end


      if (!isImmSrc2()) then
        if !(/^\d/.match(@src2) ) && !(/__cuda/.match(@src2)) && !(/(t|cta)id/.match(@src2)) && @src2 != ""
          if !(/p/.match(@src2))
            @useSet.add(@src2)
          end
        end
      end

      if (!isImmSrc3()) then
        if !(/^\d/.match(@src3) ) && !(/__cuda/.match(@src3)) && !(/(t|cta)id/.match(@src3)) && @src3 != ""
          if !(/p/.match(@src3))
            @useSet.add(@src3)
          end
        end
      end

      @useSet.delete(nil)

    end

  end
  #-----------------------------------------------------------------------------------------------------------------------#

  def whatType(line)
    a = "ToBeDefined"

    if /^\s*\/\/\s*/.match(line)
      a =  "Comment"

    elsif line == ""
      a =  "Blank"

    elsif /^\s*.(param)\s*/.match(line)
      a =  "ExtractParam"

    elsif /^\s*.(align|global|shared|union|const|local|sreg|version|entry|loc|struct|visible|extern|surf|file|reg|target|func|section|tex)\s*/.match(line)
      a =  "Directive"


    elsif /^\s*(@)/.match(line)
      # puts line
      a =  "Predicated Instruction"


    elsif /^\s*(abs|cos|min|ret|sqrt|add|cvt|mov|rsqrt|st|addc|div|mul|sad|sub|and|ex2|mul24|selp|tex|atom|exit|neg|set|trap|bar|ld|not|setp|vote|bra|lg2|or|shl|xor|brkpt|mad|rcp|shr|call|mad24|red|sin|cnot|max|rem|slct)\s*/.match(line)
      a =  "Instruction"

    elsif /^\$(\w*:)./.match(line)
      a =  "Label(this line)"

    elsif /^\$(\w*:)/.match(line)
      a =  "Label(next line)"

    elsif /^\s*(\{|\})/.match(line)
      a =  "Loop Begining/ Loop terminating"

    end

    return a
  end
  #-----------------------------------------------------------------------------------------------------------------------#


  def processsRegisterStateSpaceDirective(line)

    # 1. Check if alignment is present
    if /^\s*.align\s*/.match(line)
      #Interpret instruction as <.directive><.align> {#}<.type>.<variablename>{[number]} {= values}
      puts "Line #{line}  : Alignment\n "

      # 2. Check if it is a vector definition
    elsif /^\s*.(v)\s*/.match(line)
      #Interpret instruction as <.directive><.align> {#}<.type>.<variablename>{[number]} {= values}
      puts "Line #{line} : Vector\n"


    elsif /^\s*.reg\s*/.match(line)
      puts "Line #{line} : Register\n"


    elsif /^\s*.sreg\s*/.match(line)
      puts "Line #{line} : Special Reg\n"
    elsif /^\s*.const\s*/.match(line)
      puts "Line #{line} : Const\n"


      # 3. Normal Processing.


    else
      puts "Line #{line}  : Simple\n"
    end
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def processsEntryPointDirective(line,lineNumber)
  end
  #-----------------------------------------------------------------------------------------------------------------------#
  def processsMiscDirective(line,lineNumber)
  end
  #-----------------------------------------------------------------------------------------------------------------------#

  def processInstruction(line)
    if (DEBUG)
      #puts "\n\n#{line}"
      puts "\n\n"
    end

    # Split the Opcode.
    line = line.split(' ')
    @opcode = line [0]
    line.delete(@opcode)
    registers = ""
    line.each do |item|
      registers.concat(item)
    end
    line = registers.split(',')


    if /^\s*(add|addc|sub|mul|mul24|div|rem|min|max|and|or|xor|shl|shr)/.match(@opcode) #op d,a,b;

      #puts "registers = #{registers}\n"
      @dest = line[0]
      @dest.delete! "%"
      @src1 = line[1]

      @immSrc1 = checkImmSrc(@src1)
      @src1.delete! "%"

      @src2 = line[2]
      @immSrc2 = checkImmSrc(@src2)
      @src2.delete! "%"


      if (DEBUG)
        puts("@opcode = #{@opcode} dest = #{@dest}, @src1 = #{@src1}, @src2 = #{@src2}\n")
        #puts "a\n"
      end

    elsif /^\s*(abs|neg|not|cnot)/.match(@opcode) #op,d,a;

      @dest = line[0]
      @dest.delete! "%"

      @src1 = line[1]
      @immSrc1 = checkImmSrc(@src1)
      @src1.delete! "%"


      @src2 = ""
      if (DEBUG)
        puts("@opcode = #{@opcode} @dest = #{@dest}, @src1 = #{@src1}, @src2 = #{@src2}\n")
        # puts "b\n"
      end

    elsif /^\s*(setp)/.match(@opcode)
      @dest = line[0]
      @dest.delete! "%"


      @desta = @dest.split('|')[1]
      @dest = @dest.split('|')[0]

      @src1 = line[1]
      @immSrc1 = checkImmSrc(@src1)
      @src1.delete! "%"

      @src2 = line[2]
      @immSrc2 = checkImmSrc(@src2)
      @src2.delete! "%"

      @src3 = line[3]
      if (@src3 != nil)
        @immSrc3 = checkImmSrc(@src3)
        @src3.delete! "%"
      end

      if (DEBUG)
        puts("@opcode = #{@opcode} @dest = #{@dest}, @desta = #{@desta}, @src1 = #{@src1}, @src2 = #{@src2}, src3 = #{@src3}\n")
        #puts "d\n"
      end
    elsif /^\s*(mad|mad24|sad|selp|slct|set)/.match(@opcode)#op,d,a,b,c;
      if !(/selp/.match(@opcode)) then
        @dest = line[0]
        @dest.delete! "%"

        @src1 = line[1]
        @immSrc1 = checkImmSrc(@src1)
        @src1.delete! "%"

        @src2 = line[2]
        @immSrc2 = checkImmSrc(@src2)
        @src2.delete! "%"

        @src3 = line[3]
        if (@src3!= nil)
          @immSrc3 = checkImmSrc(@src3)
          @src3.delete! "%"
        end

      else
        @dest = line[1]
        @dest.delete! "%"

        @src1 = line[2]
        @immSrc1 = checkImmSrc(@src1)
        @src1.delete! "%"

        @src2 = line[3]
        @immSrc2 = checkImmSrc(@src2)
        @src2.delete! "%"
      end


      if (DEBUG)
        puts("@opcode = #{@opcode} @dest = #{@dest}, @src1 = #{@src1}, @src2 = #{@src2}, @src3 = #{@src3}\n")
      end
      #puts "c\n"



    elsif /^\s*(mov|ld|st|cvt)/.match(@opcode)
      if /^\s*(st)/.match(@opcode)
        #Source has no destination, both are sources        
        @src1 = line[0]
        if (@src1 != nil)
          @immSrc1 = checkImmSrc(@src1)
          @src1.delete! "%"
        end

        @src2 = line[1]
        if (@src2 != nil)
          @immSrc2 = checkImmSrc(@src2)
          @src2.delete! "%"
        end

#        @dest = line[1]
#        if (@dest != nil)
#          @dest.delete! "%"
#        end

      else
        @dest = line[0]
        @dest.delete! "%"
        @src1 = line[1]
        if (@src1 != nil)
          @immSrc1 = checkImmSrc(@src1)
          @src1.delete! "%"
        end
      end

      if (DEBUG)
        puts("@opcode = #{@opcode} @dest = #{@dest}, @src1 = #{@src1}\n")
        #puts "e\n"
      end

    elsif /^\s*(bra|call|ret|exit)/.match(@opcode)
      @dest = line[0]
      if (@dest != nil)
        @dest.delete! "%"
      end

      @src1 = line[1]
      if (@src1 != nil)
        @immSrc1 = checkImmSrc(@src1)
        @src1.delete! "%"
     end

      if (DEBUG)
        puts("@opcode = #{@opcode} @dest = #{@dest}, @src1 = #{@src1}\n")
        #puts "f\n"
      end

    elsif /^\s*(bar|atom|red|vote)/.match(@opcode)
      @dest = line[0]
      @dest.delete! "%"

      @src1 = line[1]
      if (@src1 != nil)
        @immSrc1 = checkImmSrc(@src1)
        @src1.delete! "%"
      end

      if (DEBUG)
        puts("@opcode = #{@opcode} @dest = #{@dest}, @src1 = #{@src1}\n")
        #puts "g\n"
      end

    elsif /^\s*(rcp|sqrt|rsqrt|sin|cos|lg2|ex2)/.match(@opcode)
      @dest = line[0]
      @dest.delete! "%"

      @src1 = line[1]
      @immSrc1 = checkImmSrc(@src1)
      @src1.delete! "%"

      if (DEBUG)
        puts("@opcode = #{@opcode} @dest = #{@dest}, @src1 = #{@src1}\n")
        #puts "h\n"
      end

    elsif /^\s*(trap|brkpt)/.match(@opcode)
      #Nothing here
      if (DEBUG)
        puts("@opcode = #{@opcode}\n")
      end
    end

    #TODO : Are we checking for "ALL" right opcodes?
    #if /^\s*(@|bar|bra|call|ret)\s*/.match(getOpcode()) #BRA,CALL,RET,EXIT,@,}
    if /^\s*(#{$PTX_LEADER_INSTRUCTIONS})\s*/.match(getOpcode()) #BRA,CALL,RET,EXIT,@,}
      setAsLeader()
      # puts getInst
      # puts "&&&&"
    end


    if /\+/.match(@src1)
      @src1Off = @src1.split('+')[1]
      @src1Off.delete! "]"
      @src1 = @src1.split('+')[0]
      @src1.delete! "["
    end

    if /\+/.match(@src2)
      @src2Off = @src2.split('+')[1]
      @src2Off.delete! "]"
      @src2 = @src2.split('+')[0]
      @src2.delete! "["
    end

    if /\+/.match(@src3)
      @src3Off = @src3.split('+')[1]
      @src3Off.delete! "]"
      @src3 = @src3.split('+')[0]
      @src3.delete! "["
    end

    if /\+/.match(@dest)
      @destOff = @dest.split('+')[1]
      @destOff.delete! "]"
      @dest = @dest.split('+')[0]
      @dest.delete! "["
    end
  end
  #-----------------------------------------------------------------------------------------------------------------------#

def checkImmSrc(src)
  # Immediate data is not "immediate" as such. It can stand even for the constant data that we see
  # any source that is not marked IMM, will be used in the LILO calculations. 
  # (Immediate numbers, constant variables.. shouldn't be)

  if (!(/__cuda/.match(src)) && !(/%/.match(src)) && !($PTX_PREDEFINED_IDENTIFIERS.include?(src)))
    
    #localstr = src
    #localstr.delete! "%"
    #if /\+/.match(localstr)
    #  localstr = localstr.split('+')[0]
    #  localstr.delete! "["
    #end
    #if ($programconstants["#{localstr}"] == nil)
    #  puts "-----%%%%%%%--------->   #{src}"
    #  return true
    #else
    #  return false
    #end

    #puts "-----%%%%%%%--------->   #{src}"
    return true
  end
end

end
