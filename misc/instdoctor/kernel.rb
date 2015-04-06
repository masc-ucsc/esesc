############################################################################
#    Copyright (C) 2010 by Alamelu Sankaranarayanan                        #
#    alamelu@cse.ucsc.edu                                                  #
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

class PTXKernel
  attr_accessor :kernel_name, :kernel_lastlabel, :kernel_decln_start, :kernel_line_start, :kernel_line_end, :kernel_params
  attr_accessor :kernel_bb, :kernel_instDecode, :kernel_addressmap, :kernel_shmem_map, :kernel_labels, :kernel_TRACECTXPTR_32,:kernel_TRACECTXPTR_64, :kernel_TRACECTXPTR_FP, :kernel_SHMPTR_32, :kernel_SHMPTR_64, :kernel_SHMPTR_FP,:kernel_SHMPTR_ADDR, :kernel_FIRSTBB, :kernel_lastlocation, :kernel_lastLabel, :kernel_BBCP32_pos, :kernel_BBCPFP_pos,:kernel_BBCP64_pos, :kernel_SHM32_pos, :kernel_SHMFP_pos, :kernel_SHM64_pos, :kernel_SHM_addr, :kernel_registers

  def initialize(entrypoint)

    # Reset all the global variables
    reset_global_variables()

    #Extract the kernelname and kernel parameters
    @kernel_decln_start = $number_of_lines

    firstlineincomplete = true


    inst = Instruction.new(entrypoint)
    if (inst.getLabel)
      $labels["#{inst.getLabel}"] = Label.new($number_of_lines,inst.getLabel)
      $lastLabel = inst.getLabel
    end
    $instDecode["#{$number_of_lines}"] = inst
    $number_of_lines += 1

    while (firstlineincomplete)
      #puts entrypoint

      if ((/\(/.match(entrypoint)) && (/\{/.match(entrypoint))) then
          firstlineincomplete = false
      else
        $filein.gets
        #puts $_
        if ($_ != nil) then
          $_.rstrip!
          $_.lstrip!
        end
        inst = Instruction.new($_)
        if (inst.getLabel)
          $labels["#{inst.getLabel}"] = Label.new($number_of_lines,inst.getLabel)
          $lastLabel = inst.getLabel
          #     			puts $lastLabel

        end
        $instDecode["#{$number_of_lines}"] = inst
        $number_of_lines += 1
        entrypoint += $_
      end
    end

    #Now extract the name of the kernel
    @kernel_name = entrypoint.split('(')[0]
    @kernel_name = @kernel_name.split('.entry')[1]
    @kernel_name.lstrip! and @kernel_name.rstrip!
    #puts @kernel_name

    #Extract the various parameters
    @kernel_params = Set.new

    if (!(entrypoint == "") && !(entrypoint != nil)) then 
      entrypoint = entrypoint.split('(')[1]
      entrypoint = entrypoint.split(')')[0]
    
      temp = entrypoint.split(',')
      temp.each do |item|
        item = item.split('.param')[1] and item.lstrip! and item.rstrip!
        kernelparam = PTXParamType.new
        kernelparam.paramname = item.split(/\s/)[1]
        kernelparam.paramwidth = item.split(/\s/)[0]
        @kernel_params |= kernelparam
      end
    end
    if ((@kernel_params.size() == 0) && /\{/.match(@kernel_name)) then
      @kernel_name.delete! "{"
      #kernelparam = PTXParamType.new
      #kernelparam.paramname = "{" 
      #@kernel_params |= kernelparam
    end

    # Read the file till you reach the end of the kernel
    kernel_end = false
    @kernel_line_start = $number_of_lines
    # 		puts @kernel_line_start

    while (!kernel_end)

      $filein.gets
      $_.rstrip! and $_.lstrip!
      #puts $_
      if /\}/.match($_)
        if !/;/.match($_)
          kernel_end = true
          @kernel_line_end = $number_of_lines
          #  puts @kernel_line_end
        end
      end

      inst = Instruction.new($_)

      if (inst.getLabel)
        $labels["#{inst.getLabel}"] = Label.new($number_of_lines,inst.getLabel)
        #  puts inst.getLabel
        $lastLabel = inst.getLabel
        if (/#{@kernel_name}/.match($lastLabel) && !/end/.match($lastLabel)) then
        inst.setAsLeader
      end
    end
    $instDecode["#{$number_of_lines}"] = inst
    $number_of_lines += 1
  end # end of while loop

  if (DEBUG) then
    puts "%%%%%%%%%%%%%%%%%%"
    $labels.each {|key, value| puts "#{key} is #{value.inspect}" }
    puts "%%%%%%%%%%%%%%%%%%"
    $SHM_addr = $SHM32_pos+$SHM64_pos+$SHMFP_pos
    puts "There are #{$SHM32_pos} ints, #{$SHM64_pos} long ints, #{$SHMFP_pos} floats and #{$SHM_addr} stores to shared mem \n\n"
  end

  #Copy all the global variables to the local variables
  #puts $predicates.inspect()
  copy_global_to_local()

end

def copy_local_to_global()
  $bb = @kernel_bb
  $instDecode = @kernel_instDecode
  $address_map = @kernel_addressmap
  $shmem_map = @kernel_shmemmap
  $labels = @kernel_labels

  $TRACECTXPTR_32 = @kernel_TRACECTXPTR_32
  $TRACECTXPTR_64 = @kernel_TRACECTXPTR_32
  $TRACECTXPTR_FP = @kernel_TRACECTXPTR_FP
  $SHMPTR_32 = @kernel_SHMPTR_32
  $SHMPTR_64 = @kernel_SHMPTR_64
  $SHMPTR_FP = @kernel_SHMPTR_FP
  $SHMPTR_ADDR = @kernel_SHMPTR_ADDR

  $FIRSTBB = @kernel_FIRSTBB
  $lastlocation = @kernel_lastlocation
  $lastLabel = @kernel_lastLabel

  $BBCP32_pos = @kernel_BBCP32_pos
  $BBCPFP_pos = @kernel_BBCPFP_pos
  $BBCP64_pos = @kernel_BBCP64_pos

  $SHM32_pos = @kernel_SHM32_pos
  $SHMFP_pos = @kernel_SHMFP_pos
  $SHM64_pos = @kernel_SHM64_pos
  $SHM_addr= @kernel_SHM_addr
end


def copy_global_to_local()
  @kernel_bb = $bb
  @kernel_instDecode = $instDecode
  @kernel_addressmap = $addressmap
  @kernel_shmemmap = $shmemmap
  @kernel_labels = $labels

  @kernel_TRACECTXPTR_32 = $TRACECTXPTR_32
  @kernel_TRACECTXPTR_64 = $TRACECTXPTR_32
  @kernel_TRACECTXPTR_FP = $TRACECTXPTR_FP
  @kernel_SHMPTR_32 = $SHMPTR_32
  @kernel_SHMPTR_64 = $SHMPTR_64
  @kernel_SHMPTR_FP = $SHMPTR_FP
  @kernel_SHMPTR_ADDR = $SHMPTR_ADDR

  @kernel_FIRSTBB = $FIRSTBB
  @kernel_lastlocation = $lastlocation
  @kernel_lastLabel = $lastLabel

  @kernel_BBCP32_pos = $BBCP32_pos
  @kernel_BBCPFP_pos = $BBCPFP_pos
  @kernel_BBCP64_pos = $BBCP64_pos

  @kernel_SHM32_pos = $SHM32_pos
  @kernel_SHMFP_pos = $SHMFP_pos
  @kernel_SHM64_pos = $SHM64_pos
  @kernel_SHM_addr= $SHM_addr
end

def reset_global_variables()
  $bb = Hash.new                 # KEY: BBID, VALUE: BB
  $instDecode = Hash.new         # KEY: LINE NUMBER, VALUE: (DECODED) INSTRUCTION
  $addressmap = Hash.new         # KEY: register, Value: Position in the respective array
  $successors = SortedSet.new
  $shmemmap = Hash.new           # Key: Address, Value: Instructions to store the value into Shmem pointer
  $labels = Hash.new
  $registers = Hash.new					 # Key: register, Value: type
  $predicates = Hash.new
#  $programconstants = Hash.new					 # Key: variable, Value: type  # DOnt clear this.. cos other wise const labels which are declared before the prog are erased.

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

if ($M32)
	$BBTPOFFSET = 48
else
	$BBTPOFFSET = 96
end

  $FIRSTBB = 1                                   #Set  to specify where the basic block count should begin from. Strictly > 0
  $lastlocation = $BBTPOFFSET                    #points To The Location In The Tracepointer Where The Last Mem Ref Was Stored.
  $lastLabel = String.new

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

end

def reset_global_liveanal_variables()
  #Used for Live Variable Analysis only
  #---------------------------------------------------------------------#
  $USE = Array.new
  $DEF = Array.new
  $LIVEIN = Array.new
  $LIVEOUT = Array.new
  $LIVEINDASH = Array.new
  $LIVEOUTDASH = Array.new
  $LINENO = Array.new
end


def outline_basic_blocks()
  #--------------------------------------------------------------------------------------------#
  # Some leaders are already identified when the instructions were being processed.
  # Some more will be identified based on destinations of jumps, branches
  # Some Basic Blocks will be split if the number of traces to be copied back to the host exceeds a predefined number.
  #-------------------------------------------------------------------------------------------#
  local_counter_i = @kernel_line_start
  #  puts @kernel_line_start
  #  puts @kernel_line_end

  while (local_counter_i < @kernel_line_end)
    #puts local_counter_i
    if @kernel_instDecode.has_key?("#{local_counter_i}")
      if (/^\s*(@|bra|call)\s*/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode)) #BRA,CALL,RET,EXIT,@,}
        if (DEBUG)
          puts @kernel_labels["#{@kernel_instDecode["#{local_counter_i}"].getDest}"].inspect
        end
        @kernel_instDecode["#{@kernel_labels["#{@kernel_instDecode["#{local_counter_i}"].getDest}"].getLineNumber}"].setAsLeader
      end
    end
    local_counter_i += 1
  end

  @kernel_bb = Hash.new

  local_load_counter = 0
  local_store_counter = 0
  local_inst_counter = 0
  local_trace_array_size = 0
  local_tempBBID = @kernel_FIRSTBB - 1
  local_bb_start_line_number = @kernel_line_start
  local_use_SHM = false
  local_use_pred = Array.new

  local_counter_i = @kernel_line_start
  @kernel_TRACESIZE = 0

  dbg1 = false
  while (local_counter_i <= @kernel_line_end)
#    if (local_counter_i >= 325 && local_counter_i <=328) then
#      dbg1 = true
#    else
#      if (dbg1)
#        exit
#      else
#        dbg1 = false
#      end
#    end

    if (dbg1) then
      puts "#{local_counter_i}"
    end

    if @kernel_instDecode.has_key?("#{local_counter_i}")
      if (@kernel_instDecode["#{local_counter_i}"].getPred() != nil)
        if (dbg1) then
          puts "Here!!"
        end
        local_use_pred.insert(local_use_pred.size(), @kernel_instDecode["#{local_counter_i}"].getPred())
      end

      if ((@kernel_instDecode["#{local_counter_i}"].isLeader) || (local_trace_array_size >= $MAXTRACESIZE) || (local_inst_counter >= $MAXINSTPERBB))
        if (DEBUG)
          puts "#{(@kernel_instDecode["#{local_counter_i}"].isLeader)} #{(local_trace_array_size >= $MAXTRACESIZE)} #{(local_inst_counter >= $MAXINSTPERBB)}"
        end

        if (@kernel_instDecode["#{local_counter_i}"].isInst)
          if /ld/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode())
            if !/(__cudaparm|__cuda_local)/.match(@kernel_instDecode["#{local_counter_i}"].getSrc1())
              #puts "#{@kernel_instDecode["#{local_counter_i}"].getInst()}"
              local_load_counter += 1
              local_trace_array_size += 1
            end
            if /shared/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode())
              local_use_SHM = true
            end
          elsif /st/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode())
            if !/(__cudaparm|__cuda_local)/.match(@kernel_instDecode["#{local_counter_i}"].getSrc1())
              #puts "#{@kernel_instDecode["#{local_counter_i}"].getInst()}"
              local_store_counter += 1
              local_trace_array_size += 1
            end
            if /shared/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode())
              local_use_SHM = true
            end
          end
        end
        if (/bar/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode()))
          # End the previous basic block before the barrier, ASSUMING that there were valid instructions.
          if (local_inst_counter > 0)
            @kernel_bb["#{local_tempBBID}"] = BasicBlock.new(local_tempBBID,local_bb_start_line_number,local_counter_i-1,@kernel_instDecode["#{local_bb_start_line_number}"].getLabel,local_use_SHM,local_inst_counter,local_use_pred)
            @kernel_bb["#{local_tempBBID}"].setLoadCount(local_load_counter)
            @kernel_bb["#{local_tempBBID}"].setStoreCount(local_store_counter)
            if (@kernel_TRACESIZE <(local_load_counter+local_store_counter))
              @kernel_TRACESIZE = (local_load_counter+local_store_counter)
            end
            local_tempBBID += 1
            local_load_counter = 0
            local_store_counter = 0
            #puts ".................#{local_tempBBID} 1-->#{local_trace_array_size} + 10  + #{$predicates.size()} is the size of the the trace pointer needed"
            local_trace_array_size = 0
            local_inst_counter = 0
            local_use_SHM = false
            local_use_pred = Array.new
            local_bb_start_line_number = local_counter_i
          end

          # Put the barrier in a basic block of its own.
          # local_bb_start_line_number = local_counter_i
          @kernel_bb["#{local_tempBBID}"] = BasicBlock.new(local_tempBBID,local_bb_start_line_number,local_counter_i,@kernel_instDecode["#{local_bb_start_line_number}"].getLabel,local_use_SHM,1,local_use_pred)
          @kernel_bb["#{local_tempBBID}"].setLoadCount(local_load_counter)
          @kernel_bb["#{local_tempBBID}"].setStoreCount(local_store_counter)
          if (@kernel_TRACESIZE <(local_load_counter+local_store_counter))
            @kernel_TRACESIZE = (local_load_counter+local_store_counter)
          end
          local_tempBBID += 1
          local_load_counter = 0
          local_store_counter = 0
          #puts ".................#{local_tempBBID} 2-->#{local_trace_array_size} + 10  + #{$predicates.size()} is the size of the the trace pointer needed"
          local_trace_array_size = 0
          local_inst_counter = 0
          local_use_SHM = false
          local_use_pred = Array.new

          # Start the next basic block from the line after the barrier
          local_bb_start_line_number = local_counter_i+1
        else

          if (local_tempBBID < @kernel_FIRSTBB)
            @kernel_bb["headers"] = BasicBlock.new(local_tempBBID,@kernel_decln_start,local_counter_i-1,"headers",local_use_SHM,0,local_use_pred)
            @kernel_bb["headers"].resetstaticvariables()
            local_bb_start_line_number = local_counter_i
            local_tempBBID = @kernel_FIRSTBB
            local_load_counter = 0
            local_store_counter = 0
            #puts ".................#{local_tempBBID} 2-->#{local_trace_array_size} + 10  + #{$predicates.size()} is the size of the the trace pointer needed"
            local_trace_array_size = 0
            local_inst_counter = 0
            local_use_SHM = false
            local_use_pred = Array.new
          elsif (local_inst_counter >= 0)
            #puts "#{local_tempBBID} < #{@kernel_FIRSTBB} #{local_inst_counter} >= 0 "
            if (@kernel_instDecode["#{local_counter_i}"].getLabel)
              if (local_bb_start_line_number <= local_counter_i -1)

                #Make doubly sure that the last line was acounted for
                if (local_inst_counter == 0) then
                  if (@kernel_instDecode["#{local_counter_i-1}"].isInst)
                    local_inst_counter += 1
                  end
                end

                if (DEBUG)
                  puts "#{local_counter_i} ---->#{local_tempBBID} ----> #{@kernel_instDecode["#{local_bb_start_line_number}"].getLabel}"
                end
                @kernel_bb["#{local_tempBBID}"] = BasicBlock.new(local_tempBBID,local_bb_start_line_number,local_counter_i-1,@kernel_instDecode["#{local_bb_start_line_number}"].getLabel,local_use_SHM,local_inst_counter,local_use_pred)

                @kernel_bb["#{local_tempBBID}"].setLoadCount(local_load_counter)
                @kernel_bb["#{local_tempBBID}"].setStoreCount(local_store_counter)
                if (@kernel_TRACESIZE <(local_load_counter+local_store_counter))
                  @kernel_TRACESIZE = (local_load_counter+local_store_counter)
                end
                local_bb_start_line_number = local_counter_i
                local_tempBBID += 1
                local_load_counter = 0
                local_store_counter = 0
                #puts ".................#{local_tempBBID} 3-->#{local_trace_array_size} + 10  + #{$predicates.size()} is the size of the the trace pointer needed"
                local_trace_array_size = 0
                local_inst_counter = 0
                local_use_SHM = false
                local_use_pred = Array.new

              end
            else
              if (local_bb_start_line_number <= local_counter_i -1)
                #puts "Here3!!"
                #if (DEBUG)
                if (dbg1)
                  puts "#{local_counter_i} ----> #{local_tempBBID} -----> #{@kernel_instDecode["#{local_bb_start_line_number}"].getLabel}"
                  puts "#{local_counter_i} ----> #{local_tempBBID} -----> #{local_bb_start_line_number} --------> #{local_counter_i} --------->#{local_inst_counter}"
                end

                #Make Doubly sure that the last instruction has been accounted for
                if (local_inst_counter == 0) then
                  if (@kernel_instDecode["#{local_counter_i}"].isInst)
                    local_inst_counter += 1
                    puts "\n**********************\nIgnored:"
                    puts @kernel_instDecode["#{local_counter_i}"].getInst()
                    puts "But not anymore!\n\n"
                  end
                end

                @kernel_bb["#{local_tempBBID}"] = BasicBlock.new(local_tempBBID,local_bb_start_line_number,local_counter_i,@kernel_instDecode["#{local_bb_start_line_number}"].getLabel,local_use_SHM,local_inst_counter,local_use_pred)
                @kernel_bb["#{local_tempBBID}"].setLoadCount(local_load_counter)
                @kernel_bb["#{local_tempBBID}"].setStoreCount(local_store_counter)
                if (@kernel_TRACESIZE <(local_load_counter+local_store_counter))
                  @kernel_TRACESIZE = (local_load_counter+local_store_counter)
                end
                local_bb_start_line_number = local_counter_i+1
                local_tempBBID += 1
                local_load_counter = 0
                local_store_counter = 0
                #puts ".................#{local_tempBBID} 4--> #{local_trace_array_size} + 12  + #{$predicates.size()} is the size of the the trace pointer needed"
                local_trace_array_size = 0
                local_inst_counter = 0
                local_use_SHM = false
                local_use_pred = Array.new
              else
              end #if (local_bb_start_line_number <= local_counter_i -1)

            end # if (@kernel_instDecode["#{local_counter_i}"].getLabel)
          else
            #local_bb_start_line_number = local_counter_i+1
          end #if (local_inst_counter > 0)
        end
      elsif /ld/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode())
        if !/(__cudaparm|__cuda_local)/.match(@kernel_instDecode["#{local_counter_i}"].getSrc1())
          #puts "#{@kernel_instDecode["#{local_counter_i}"].getInst()}"
          local_load_counter += 1
          local_trace_array_size += 1
        end
        if /shared/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode())
          local_use_SHM = true
        end
      elsif /st/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode())
        if !/(__cudaparm|__cuda_local)/.match(@kernel_instDecode["#{local_counter_i}"].getSrc1())
          #puts "#{@kernel_instDecode["#{local_counter_i}"].getInst()}"
          local_store_counter += 1
          local_trace_array_size += 1
        end
        if /shared/.match(@kernel_instDecode["#{local_counter_i}"].getOpcode())
          local_use_SHM = true
        end
      end

      if (@kernel_instDecode["#{local_counter_i}"].isInst)
        local_inst_counter += 1
      else
        #puts @kernel_instDecode["#{local_counter_i}"].getInst()
      end

    end
    local_counter_i += 1
  end

  #@kernel_TRACESIZE = @kernel_TRACESIZE+4 #4 for the header //FIXME

  if (DEBUG)
    puts ".................#{@kernel_TRACESIZE} + 12 + #{$predicates.size()}  + #{$predicates.size()} is the size of the the trace pointer needed"
    puts ".................Basic Blocks #{@kernel_FIRSTBB} to #{local_tempBBID}"
    @kernel_bb.each {|basbloc| puts basbloc.inspect}
  end

end

def generateCFG
  local_counter_i = @kernel_FIRSTBB
  while (local_counter_i <= @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs+@kernel_FIRSTBB)
    if @kernel_bb.has_key?("#{local_counter_i}")
      $termblocks = @kernel_bb["#{local_counter_i}"].generateCFG(@kernel_instDecode,@kernel_labels,@kernel_bb,@kernel_FIRSTBB)
    end
    local_counter_i += 1
  end
end #End of generateCFG

def printCFG 
  local_counter_i = @kernel_FIRSTBB
  while (local_counter_i < @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs+@kernel_FIRSTBB)
    div = @kernel_bb["#{@kernel_FIRSTBB + local_counter_i - 1}"].isDivergent
    divstr = ""
    if div
      divstr = "Divergent"
    else
      divstr = "Non Divergent"
    end

    succ = @kernel_bb["#{@kernel_FIRSTBB + local_counter_i - 1}"].getSucc
    if succ.size() > 0
      puts "#{divstr} BB #{local_counter_i} goes to BB #{succ.inspect}"
    else
      puts "#{divstr} BB #{local_counter_i} is a terminal BB"
    end
    local_counter_i += 1
  end
end #End of printCFG 

def map_reconv_bbs
  puts "***********************************************"
  lastBB = @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs+@kernel_FIRSTBB - 1 
  node = @kernel_FIRSTBB + lastBB - 1
  while (node >= @kernel_FIRSTBB )

    succ = @kernel_bb["#{node}"].getSucc
    pre  = @kernel_bb["#{node}"].getPredec
    rec  = @kernel_bb["#{node}"].getreconv_node

    if pre.size() > 0
      #puts "BB #{node} has inlets from #{pre.inspect}"
    else
      #Topmost BB
      #puts "BB #{node} has no inlets"
    end

    if pre.size() > 1
      for ubb in pre
        #Check and see that there if there is a loop
        if succ.include?(ubb)
          #puts "#{ubb} is also a successor for #{node}, there is a loop between the two nodes"
          #puts "Ignoring this edge between #{ubb} and #{node}"
        else
          #puts "Setting #{node} as the reconv_node for BB #{ubb}"
          @kernel_bb["#{ubb}"].setreconv_node(node)
        end
      end
    else 
      if pre.size() == 1
        if succ.size() > 0
          #puts "Setting #{rec[0]} as the reconv_node for BB #{pre[0]}"
          @kernel_bb["#{pre[0]}"].setreconv_node(rec[0])
        else
          #puts "Node #{node} is a terminal node, It has no reconv node"
          #puts "Setting #{node} as the reconv_node for BB #{pre[0]}"
          @kernel_bb["#{pre[0]}"].setreconv_node(node)
        end
      else
        #puts "pre.size is 0, No predecessors"
      end
      unify_reconv_nodes(node,@kernel_FIRSTBB,lastBB)
    end
    node = node - 1

  end
end #End of map_reconv_bbs

def unify_reconv_nodes(bbid,firstbbid,lastbbid)
    node = firstbbid + bbid - 1
    rec  = @kernel_bb["#{node}"].getreconv_node
    if rec.size() > 1
      node = firstbbid
      while (node <= lastbbid) 
        @kernel_bb["#{node}"].markunvisited
        node += 1
      end

      node = rec[0];
      $successors.clear()
      succ_set1 = get_path_to_term(firstbbid,node)
      succ_set1.unshift(node)
#      puts succ_set1.inspect

      node = firstbbid
      while (node <= lastbbid) 
        @kernel_bb["#{node}"].markunvisited
        node += 1
      end
      node = rec[1];
      $successors.clear()
      succ_set2 = get_path_to_term(firstbbid,node)
      succ_set2.unshift(node)
#      puts succ_set2.inspect

      intersection = succ_set1 & succ_set2
      #puts "Intersection is #{intersection.inspect()}"
    
      node = firstbbid + bbid - 1
      if intersection.empty?()
        puts "ERROR!!!!!!!!, How can there be no common node ??"
      else
        @kernel_bb["#{node}"].clearreconv_node
        @kernel_bb["#{node}"].setreconv_node((intersection.to_a())[0])
        #puts "Reconvergence Node is : #{@kernel_bb["#{node}"].getreconv_node}"
      end
    end
end

def get_path_to_term(firstbb, node)
  succ = @kernel_bb["#{node}"].getSucc
  succ.each do |child|
    if @kernel_bb["#{child}"].isvisited == false
      @kernel_bb["#{child}"].markvisited()
      get_path_to_term(firstbb, firstbb+child-1)
      #puts child
      $successors.add(child)
    end
  end


  return $successors.to_a()
end

def generateLILO(bbid)
  # This function analyzies each basic block and defines the GEN and KILL set for each basic block
  # Generate the Gen Set and the Kill Set for each basic block
  $USE.clear
  $DEF.clear
  $LIVEIN.clear
  $LIVEOUT.clear

  bb_start = @kernel_bb["#{bbid}"].getStart
  bb_end = @kernel_bb["#{bbid}"].getEnd

  while (bb_start <= bb_end)
    if @kernel_instDecode.has_key?("#{bb_start}")
      if (@kernel_instDecode["#{bb_start}"].isInst)
        if ((@kernel_instDecode["#{bb_start}"].getUseSet != nil) || (@kernel_instDecode["#{bb_start}"].getDefSet != nil))
          $USE.insert($USE.size,@kernel_instDecode["#{bb_start}"].getUseSet)
          $DEF.insert($DEF.size,@kernel_instDecode["#{bb_start}"].getDefSet)
          $LIVEIN.insert($LIVEIN.size,Set.new)
          $LIVEOUT.insert($LIVEOUT.size,Set.new)
        end
      end
    end
    bb_start += 1
  end


  n = $USE.size-1

  while (n >= 0)

    # Set the Live - Outs
    if (n == $DEF.size()-1)
      $LIVEOUT[n] = $DEF[n]
    else
      $LIVEOUT[n] = $LIVEOUT[n] | $LIVEIN[n+1]
    end

    if ($LIVEOUT[n] == nil)
      $LIVEOUT[n] = Set.new
    end

    #Set the live-ins
    if ($DEF[n]!=nil)
      $LIVEIN[n] = $USE[n]|($LIVEOUT[n]-$DEF[n])
    else
      $LIVEIN[n] = $USE[n]|($LIVEOUT[n])
    end
    n = n-1
  end


  n = $USE.size-1
  realLO = Set.new
  while (n >= 0)
    realLO = realLO |   $DEF[n]
    n = n - 1
  end

  @kernel_bb["#{bbid}"].setLI($LIVEIN[0])
  @kernel_bb["#{bbid}"].setLO(realLO)
  $USE.clear
  $DEF.clear
  $LIVEIN.clear
  $LIVEOUT.clear

end

def perform_live_analysis

  $USE.clear
  $DEF.clear
  $LIVEIN.clear
  $LIVEOUT.clear
  $LIVEINDASH.clear
  $LIVEOUTDASH.clear


  $USE.insert($USE.size,Set.new)
  $DEF.insert($DEF.size,Set.new)
  $LIVEIN.insert($LIVEIN.size,Set.new)
  $LIVEOUT.insert($LIVEOUT.size,Set.new)
  $LIVEINDASH.insert($LIVEINDASH.size,Set.new)
  $LIVEOUTDASH.insert($LIVEOUTDASH.size,Set.new)

  succ = Array.new
  succ_size = 0
  local_counter_i = @kernel_FIRSTBB;
  while (local_counter_i < @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs+@kernel_FIRSTBB)
    $USE.insert($USE.size,@kernel_bb["#{local_counter_i}"].getLI)
    $DEF.insert($DEF.size,@kernel_bb["#{local_counter_i}"].getLO)
    $LIVEIN.insert($LIVEIN.size,Set.new)
    $LIVEOUT.insert($LIVEOUT.size,Set.new)
    local_counter_i += 1
  end

  change = false
  count = 0
  while (!change)
    change = true
    count+=1
    # 			puts "Run #{count} times"

    #Compute the LiveIns and Live Outs
    n = $USE.size - 1
    while (n > 0)
      # 					puts "Processing BB #{@kernel_FIRSTBB + n - 1}"

      #First Calculate the Live outs
      $LIVEOUTDASH[n] = $LIVEOUT[n]
      $LIVEOUT[n] = Set.new

      if (!@kernel_bb["#{@kernel_FIRSTBB + n - 1}"].isTermBlock)
        succ = @kernel_bb["#{@kernel_FIRSTBB + n - 1}"].getSucc
        succ_size = succ.size()-1
        while (succ_size >= 0)
          temp = @kernel_bb["#{succ.fetch(succ_size)}"].getLI
          $LIVEOUT[n] = $LIVEOUT[n] | temp
          succ_size -= 1
          #                 puts "Adding liveins of basic block #{succ.fetch(succ_size)}..#{temp.inspect}"
        end
      end
      #           puts "New Liveout :  #{$LIVEOUT[n].inspect}"

      #Then calculate the Live Ins

      $LIVEINDASH[n] = $LIVEIN[n]
      $LIVEIN[n] = Set.new
      if (n != 1)
        $LIVEIN[n] = $USE[n]|($LIVEOUT[n]-$DEF[n])
      end

      #           puts "----------------"
      #           puts "#{ $USE[n].inspect} OR \n(#{$LIVEOUT[n].inspect} \n-\n #{$DEF[n].inspect})"
      #           puts "New Livein :  #{$LIVEIN[n].inspect}\n"
      n -= 1
    end

    #Compare it to the previously generated values
    n = $USE.size - 1
    while (n > 0)
      if ($LIVEOUT[n] != $LIVEOUTDASH[n])
        change = false
      end
      if ($LIVEIN[n] != $LIVEINDASH[n])
        change = false
      end
      @kernel_bb["#{@kernel_FIRSTBB + n - 1}"].setLO($LIVEOUT[n])
      @kernel_bb["#{@kernel_FIRSTBB + n - 1}"].setLI($LIVEIN[n])
      n -= 1
    end

  end
end

def map_registers_to_trace_array
  local_counter_i = @kernel_FIRSTBB

  while (local_counter_i < @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs+@kernel_FIRSTBB)

    temp = 0
    inarray = @kernel_bb["#{local_counter_i}"].getLI.to_a()
    inarray.sort!
    while (temp < inarray.size())
      reg=inarray[temp].to_s().delete "%\""
      if !(@kernel_addressmap.has_key?("#{reg}"))
        #        puts reg
        if /f/.match(reg)
          if /d/.match(reg) then
            @kernel_addressmap["#{reg}"] = $BBCPFP64_pos
            $BBCPFP64_pos += 1
          else
            @kernel_addressmap["#{reg}"] = $BBCPFP_pos
            $BBCPFP_pos += 1
          end
        elsif /d/.match(reg)
          @kernel_addressmap["#{reg}"] = $BBCP64_pos
          $BBCP64_pos += 1
        elsif /r/.match(reg)
          @kernel_addressmap["#{reg}"] = $BBCP32_pos
          $BBCP32_pos += 1
        end
      end
      temp = temp+1
    end

    temp = 0
    outarray = @kernel_bb["#{local_counter_i}"].getLO.to_a()
    outarray.sort!
    while (temp < outarray.size())
      reg=outarray[temp].to_s().delete "%\""
      if !(@kernel_addressmap.has_key?("#{reg}"))
        #        puts reg
        if /f/.match(reg)
          if /d/.match(reg) then
            @kernel_addressmap["#{reg}"] = $BBCPFP64_pos
            $BBCPFP64_pos += 1
          else
            @kernel_addressmap["#{reg}"] = $BBCPFP_pos
            $BBCPFP_pos += 1
          end
        elsif /d/.match(reg)
          @kernel_addressmap["#{reg}"] = $BBCP64_pos
          $BBCP64_pos += 1
        elsif /r/.match(reg)
          @kernel_addressmap["#{reg}"] = $BBCP32_pos
          $BBCP32_pos += 1
        end
      end
      temp = temp+1
    end
    local_counter_i = local_counter_i+1
  end

  $DFL_REG32 = $BBCP32_pos
  $DFL_REG64 = $BBCP64_pos
  $DFL_REGFP = $BBCPFP_pos
  $DFL_REGFP64 = $BBCPFP64_pos
end

def analyze()

  # STEP 1
  outline_basic_blocks()

  # STEP 2
  generateCFG()

  # STEP 2.1
  printCFG()
  
  # STEP 2.2
  map_reconv_bbs()

  # STEP 3
  local_counter_i = @kernel_FIRSTBB

  while (local_counter_i < @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs+@kernel_FIRSTBB)
    generateLILO(local_counter_i)
    if (DEBUG) then
      puts "For Basic Block #{local_counter_i}, There are #{@kernel_bb["#{local_counter_i}"].getLI.size} live-ins and #{@kernel_bb["#{local_counter_i}"].getLO.size} live-outs"
      puts "LIVEIN = #{@kernel_bb["#{local_counter_i}"].getLI.sort.inspect}"
      puts "LIVEOUT = #{@kernel_bb["#{local_counter_i}"].getLO.sort.inspect}\n\n"
      puts "-------------------------\n\n"
    end
    local_counter_i += 1
  end

  @kernel_bb["#{@kernel_FIRSTBB}"].setLI(Set.new)

  # STEP 4
  perform_live_analysis()

  # STEP 5
  map_registers_to_trace_array()

  # STEP 6
  #print_analysis_results()
end

def print_analysis_results

  local_counter_i = @kernel_FIRSTBB
  while (local_counter_i < @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs+@kernel_FIRSTBB)
    puts "For Basic Block #{local_counter_i}, There are #{@kernel_bb["#{local_counter_i}"].getLI.size} live-ins and #{@kernel_bb["#{local_counter_i}"].getLO.size} live-outs"
    puts "Basic Block #{local_counter_i} begins at #{@kernel_bb["#{local_counter_i}"].getStart} live-ins and ends at #{@kernel_bb["#{local_counter_i}"].getEnd}"
    puts "LIVEIN = #{@kernel_bb["#{local_counter_i}"].getLI.sort.inspect}"
    puts "LIVEOUT = #{@kernel_bb["#{local_counter_i}"].getLO.sort.inspect}\n\n"
    if @kernel_bb["#{local_counter_i}"].isSHMused
      puts "Basic Block #{local_counter_i} starting at #{@kernel_bb["#{local_counter_i}"].getStart} and ending at #{@kernel_bb["#{local_counter_i}"].getEnd} uses shared memory\n"
    end
    puts "-------------------------\n\n"
    local_counter_i += 1

  end
  puts "Kernel #{@kernel_name} has basic blocks from #{@kernel_FIRSTBB} to #{@kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs+@kernel_FIRSTBB - 1}"
  puts "There are #{$BBCPFP_pos} floating point registers, #{$BBCP32_pos} integer and #{$BBCP64_pos} long ints"
  puts ".................#{@kernel_TRACESIZE}  + 12 + #{$predicates.size()} is the size of the the trace pointer needed"
  puts "There are #{$SHM32_pos} ints, #{$SHM64_pos} long ints, #{$SHMFP_pos} floats and #{$SHM_addr} stores to shared mem \n\n"

end

def write_to_file(outfileptr)

  # Insert Header code
  linestart = @kernel_bb["headers"].getStart
  lineend = @kernel_bb["headers"].getEnd
  while (linestart <= lineend)
    if @kernel_instDecode.has_key?("#{linestart}")
      outfileptr.puts "#{@kernel_instDecode["#{linestart}"].getInst}\n"
      # 						  puts "#{@kernel_instDecode["#{linestart}"].getInst}\n"
    end
    linestart = linestart+1
  end

  # Insert BB initialization code
  Insert_bb_registers(outfileptr)
  #     Insert_bb_registers($stdout)

  #Insert the contaminated kernel
  Insert_contaminated_kernel(outfileptr)
  #     Insert_contaminated_kernel($stdout)
end

def Insert_bb_registers(fileptr)
  fileptr.puts "\n// BBT regs begin\n"
  fileptr.puts "\t.reg .u32 %tidx, %tidy, %ntidx, %ntidy, %ctaidx, %ctaidy, %bboff, %bbaux, %sm32data, %scratchreg1, %scratchreg2;\n"

  if ($M32)
    fileptr.puts "\t.reg .u32 %bbaux64, %bbtp, %bbcp32, %bbcp64, %bbcpfp, %bbcpfp64, %smcp32, %smcpfp, %smcpfp64, %smcp64, %smcpaddr, %smaddr, %sm64data;\n"
    fileptr.puts "\t.reg .s32 %skipinstcount, %instskipcount;\n"
  else
    fileptr.puts "\t.reg .u64 %bbaux64, %bbtp, %bbcp32, %bbcp64, %bbcpfp, %bbcpfp64, %smcp32, %smcpfp, %smcpfp64, %smcp64, %smcpaddr, %smaddr, %sm64data;\n"
    fileptr.puts "\t.reg .s64 %skipinstcount, %instskipcount;\n"
  end

  fileptr.puts "\t.reg .f32 %smfpdata;\n"
  fileptr.puts "\t.reg .f64 %smfp64data;\n"

  #Add as many predicates as the number of basic blocks
  totalbb = 0
  tmpstr = String.new("\t.reg .pred %pinst_skip, %pskip_inst, %p0_bbt;")

#  while (totalbb <= @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs-1)
#    tmpstr = tmpstr+"%p#{totalbb}_bbt, "
#    totalbb = totalbb + 1
#  end

#  tmpstr = tmpstr+"%p#{totalbb}_bbt;"
  fileptr.puts tmpstr
  fileptr.puts "//BBT regs end\n\n"
end

def Insert_BB_initialize_code(fileptr)

  #Clearing everything after that, at least for now
  #temp = 10+$predicates.size()
  temp = 12+$predicates.size()
  ######puts $predicates.size()
  ######puts @kernel_TRACESIZE
  trc_ptr_size = temp+@kernel_TRACESIZE

  fileptr.puts "\t// BBT startup code begin"
  fileptr.puts "\t// %bboffset = ((by*gridDim.x*BLOCKSIZE+bx*BLOCKSIZE+tx)*BLOCKSIZE+ty)*TRACESIZE"
  fileptr.puts "\tcvt.u32.u16     %bbaux, %nctaid.x;  //  "
  fileptr.puts "\tmov.u32         %ntidx, %ntid.x;  //"
  fileptr.puts "\tmov.u32         %ntidy, %ntid.y;  // "

  fileptr.puts "\tcvt.s32.u16     %ctaidy, %ctaid.y;      //  "
  fileptr.puts "\tcvt.s32.u16     %ctaidx, %ctaid.x;      //  "
  fileptr.puts "\tcvt.s32.u16     %tidx, %tid.x;      //  "
  fileptr.puts "\tcvt.s32.u16     %tidy, %tid.y;      //  "

  fileptr.puts "\tmul.lo.u32  %bbaux, %bbaux, %ctaidy;    //"
  fileptr.puts "\tadd.u32     %bbaux, %bbaux, %ctaidx;        //"
  fileptr.puts "\tmov.u32     %scratchreg1, %bbaux;        //"
  #fileptr.puts "\tmul.lo.u32  %bbaux, %bbaux, #{$BLOCKSIZE};     // Multiplied by the BLOCKSIZE"
  #   fileptr.puts "\tmul.lo.u32  %bbaux, %bbaux, #{$BLOCKSIZEY};     // Multiplied by the BLOCKSIZE"
  fileptr.puts "\tmul.lo.u32  %bbaux, %bbaux, %ntidx;     // Multiplied by the BLOCKSIZE"
  fileptr.puts "\tadd.u32     %bbaux, %bbaux, %tidx;      //"
  #   fileptr.puts "\tmul.lo.u32  %bbaux, %bbaux, #{$BLOCKSIZE};     // Multiplied by the BLOCKSIZE"
  #   fileptr.puts "\tmul.lo.u32  %bbaux, %bbaux, #{$BLOCKSIZEX};     // Multiplied by the BLOCKSIZE"
  fileptr.puts "\tmul.lo.u32  %bbaux, %bbaux, %ntidy;     // Multiplied by the BLOCKSIZE"
  fileptr.puts "\tadd.u32     %bboff, %bbaux, %tidy;" #%bboff has the universal scratchreg2
  fileptr.puts "\tmov.u32     %scratchreg2, %bboff;        //"
  fileptr.puts "\n\t// setting trace pointer %bbtp"

  if ($M32)
    fileptr.puts "\tld.param.u32    %bbtp, [#{$TRACEPTR}];"
  else
    fileptr.puts "\tld.param.u64    %bbtp, [#{$TRACEPTR}];"
  end

  fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{trc_ptr_size}; // TRACESIZE = #{trc_ptr_size}  MUST MATCH VALUE IN gpgpu_threads.h"

  if ($M32)
    fileptr.puts "\tcvt.u32.s32		%bbaux64, %bbaux;       //"
    fileptr.puts "\tmul.lo.u32		%bbaux64, %bbaux64, 4;  // 4 bytes per address in the trace pointer"
    fileptr.puts "\tadd.u32		%bbtp, %bbtp, %bbaux64; //"
  else
    fileptr.puts "\tcvt.u64.s32     %bbaux64, %bbaux;       //"
    fileptr.puts "\tmul.lo.u64  %bbaux64, %bbaux64, 8;  // 8 bytes per address in the trace pointer"
    fileptr.puts "\tadd.u64     %bbtp, %bbtp, %bbaux64; //"
  end


  #  fileptr.puts "\n\t// cleaning up the structure, except the BB identifier"
  #  fileptr.puts "\t// The first time this is useless: we've already cleaned the structure"
  #  fileptr.puts "\t// and set BBid to 1 in host code, before entering the computation loop"
  #  fileptr.puts "\t// Currently preserving ctid and tid too for testing purposes"
  #  fileptr.puts "\t// No loop. Don't know if mem conflicts are more harmful that a loop here"

  #  while (temp < trc_ptr_size )
  #    fileptr.puts "\tst.global.u64   [%bbtp+#{temp*8}], 0;"
  #    temp = temp+1
  #  end


  fileptr.puts "\n\n\t// Storing the BLOCKID (CTAID)"
  #   fileptr.puts "\tcvt.u32.u16     %bbaux, %nctaid.x;  //"
  #   fileptr.puts "\tcvt.s32.u16     %ctaidy, %ctaid.y;      //"
  #   fileptr.puts "\tcvt.s32.u16     %ctaidx, %ctaid.x;      //"
  #   fileptr.puts "\tmul.lo.u32  %bbaux, %bbaux, %ctaidy;//"
  #   fileptr.puts "\tadd.u32     %bbaux, %bbaux, %ctaidx; //"
  if ($M32)
    fileptr.puts "\tst.global.u32   [%bbtp+20], %scratchreg1;    //"
  else
    fileptr.puts "\tmov.u32   %bbaux, %scratchreg1;    //"
    fileptr.puts "\tcvt.u64.u32   %bbaux64, %bbaux;    //"
    fileptr.puts "\tst.global.u64   [%bbtp+40], %bbaux64;    //"
  end
  fileptr.puts "\t//%scratchreg1 will now be used as the branch count"; 

  #   if ($M32)
  # 	 fileptr.puts "\tcvt.u32.u32     %bbaux64, %scratchreg1;       //"
  # 	 fileptr.puts "\tst.global.u32   [%bbtp+20], %bbaux64;    //"
  #   else
  # 	 fileptr.puts "\tcvt.u64.u32     %bbaux64, %scratchreg1;       //"
  # 	 fileptr.puts "\tst.global.u64   [%bbtp+40], %bbaux64;    //"
  #   end

  fileptr.puts "\n\n\t// Storing the THREADID (TID)"
  #   if ($M32)
  # 	 fileptr.puts "\tcvt.u32.u32     %bbaux64, %tid;       //"
  # 	 fileptr.puts "\tst.global.u32   [%bbtp+24], %bbaux64;    //"
  #   else
  # 	 fileptr.puts "\tcvt.u64.u32     %bbaux64, %tid;       //"
  # 	 fileptr.puts "\tst.global.u64   [%bbtp+48], %bbaux64;    //"
  #   end
  if ($M32)
    fileptr.puts "\tst.global.u32   [%bbtp+24], %scratchreg2;    //"
  else
    fileptr.puts "\tmov.u32   %bbaux, %tid;    //"
    fileptr.puts "\tcvt.u64.u32   %bbaux64, %bbaux;    //"
    fileptr.puts "\tst.global.u64   [%bbtp+48], %bbaux64;    //"
  end
  fileptr.puts "\t//%scratchreg2 will now be used as the taken branch count"; 



  fileptr.puts "\n\n\t// Storing the SMID"
  fileptr.puts "\tmov.u32   %bbaux, %smid;    //"
  if ($M32)
    fileptr.puts "\tst.global.u32   [%bbtp+28], %bbaux;    //"
  else
    fileptr.puts "\tcvt.u64.u32   %bbaux64, %bbaux;    //"
    fileptr.puts "\tst.global.u64   [%bbtp+56], %bbaux64;    //"
  end


  fileptr.puts "\n\n\t// Storing the WarpID"
  #   if ($M32)
  # 	 fileptr.puts "\tcvt.u32.u32     %bbaux64, %warpid;       //"
  # 	 fileptr.puts "\tst.global.u32   [%bbtp+32], %bbaux64;    //"
  #   else
  # 	 fileptr.puts "\tcvt.u64.u32     %bbaux64, %warpid;       //"
  # 	 fileptr.puts "\tst.global.u64   [%bbtp+64], %bbaux64;    //"
  #   end
  fileptr.puts "\tmov.u32   %bbaux, %warpid;    //"
  if ($M32)
    fileptr.puts "\tst.global.u32   [%bbtp+32], %bbaux;    //"
  else
    fileptr.puts "\tcvt.u64.u32   %bbaux64, %bbaux;    //"
    fileptr.puts "\tst.global.u64   [%bbtp+64], %bbaux64;    //"
  end


  if ($M32)
    fileptr.puts "\t\n\n// Setting context context pointers "
    fileptr.puts "\n\tld.param.u32    %bbcp32, [#{$TRACECTXPTR_32}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$DFL_REG32}; // #{$DFL_REG32} regs per thread   MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u32     %bbcp32, %bbcp32, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u32    %bbcp64, [#{$TRACECTXPTR_64}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$DFL_REG64}; // #{$DFL_REG64} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 8;  //  "
    fileptr.puts "\tadd.u32     %bbcp64, %bbcp64, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u32    %bbcpfp, [#{$TRACECTXPTR_FP}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$DFL_REGFP}; // #{$DFL_REGFP} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u32     %bbcpfp, %bbcpfp, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u32    %bbcpfp64, [#{$TRACECTXPTR_FP64}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$DFL_REGFP64}; // #{$DFL_REGFP64} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 8;  //  "
    fileptr.puts "\tadd.u32     %bbcpfp64, %bbcpfp64, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u32    %smcpfp, [#{$SHMPTR_FP}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHMFP_pos}; // #{$SHMFP_pos} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u32     %smcpfp, %smcpfp, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u32    %smcpfp64, [#{$SHMPTR_FP64}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHMFP64_pos}; // #{$SHMFP64_pos} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 8;  //  "
    fileptr.puts "\tadd.u32     %smcpfp64, %smcpfp64, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u32    %smcp32, [#{$SHMPTR_32}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHM32_pos}; // #{$SHM32_pos} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u32     %smcp32, %smcp32, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u32    %smcp64, [#{$SHMPTR_64}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHM64_pos}; // #{$SHM64_pos} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 8;  //  "
    fileptr.puts "\tadd.u32     %smcp64, %smcp64, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u32    %smcpaddr, [#{$SHMPTR_ADDR}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHM_addr}; // #{$SHM_addr} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u32.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u32  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u32     %smcpaddr, %smcpaddr, %bbaux64; //  "


    #Restore shared memory for all threads .. Do this because there is no guarantee that the shared memory is going to stay consistent, 
    #since blocks might be split based on how many threads are in Timing and how many in Rabbit, and there is no way to 
    #ensure that the same block goes to the same SM inside the physical GPGPU.
    fileptr.puts "\n// Restore Shared Context\n"
    poscount = 0
    while (poscount < $SHM_addr)
      fileptr.puts "#{$shmemmap["#{poscount}"]}\n"
      poscount += 1
    end
    fileptr.puts "// **********************\n\n"

    # Add instructions to enable pausing execution.
    fileptr.puts "\n\t// Checking if execution is to be paused and jumping"
    fileptr.puts "\tld.global.u32   %bbaux64, [%bbtp+16];"
    fileptr.puts "\tsetp.eq.u32 %p0_bbt, %bbaux64, 1;"
    fileptr.puts "\t@%p0_bbt bra #{$lastLabel};"




    # Add instructions to enable pausing execution.
    fileptr.puts "\n\t// Checking if thread has completed execution"
    fileptr.puts "\tld.global.u32   %bbaux64, [%bbtp+0];"
    fileptr.puts "\tsetp.eq.u32 %p0_bbt, %bbaux64, 0;"
    fileptr.puts "\t@%p0_bbt bra #{$lastLabel};"

    # Load skip instructions.
    fileptr.puts "\n\t// Checking if native fastforwarding is enabled"
    fileptr.puts "\tld.global.s32   %skipinstcount, [%bbtp+8];"
    fileptr.puts "\tsetp.gt.s32 %pinst_skip, %skipinstcount, 0;"

    fileptr.puts "\tmov.s32   %instskipcount, 0;\n\n"

    # Load instructions Branch and Taken Branch count.
    fileptr.puts "\n\t// Loading the branch count"
    fileptr.puts "\tld.global.u32   %scratchreg1, [%bbtp+40];"

    fileptr.puts "\n\t// Loading the taken branch count"
    fileptr.puts "\tld.global.u32   %scratchreg2, [%bbtp+44];"

    fileptr.puts "\n\t// loading BB identifier and jumping"
    fileptr.puts "\tld.global.u32   %bbaux64, [%bbtp+0];"

    fileptr.puts "\n\t// saving current BB identifier and jumping"
    fileptr.puts "\tst.global.u32   [%bbtp+4], %bbaux64;"

    totalbb = @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs


##################################################################
#    # 		temp = 1
#    temp = @kernel_FIRSTBB
#    while (temp <= totalbb)
#      fileptr.puts "\tsetp.eq.u32 %p#{temp}_bbt, %bbaux64, #{temp};"
#      temp = temp+1
#    end
#    fileptr.puts "\n\n"
#    #     temp = 1
#    temp = @kernel_FIRSTBB
#    while (temp <= totalbb)
#      label = @kernel_bb["#{temp}"].getBBlabel
#      fileptr.puts "\t@%p#{temp}_bbt bra #{label};"
#      temp = temp+1
#    end
##################################################################
    temp = @kernel_FIRSTBB
    while (temp <= totalbb)
      fileptr.puts "\tsetp.eq.u32 %p0_bbt, %bbaux64, #{temp};"
      label = @kernel_bb["#{temp}"].getBBlabel
      fileptr.puts "\t@%p0_bbt bra #{label};\n"
      temp = temp+1
    end

  else #if we are compiling for 64 bit addressess
    fileptr.puts "\t\n\n// Setting context context pointers "
    fileptr.puts "\n\tld.param.u64    %bbcp32, [#{$TRACECTXPTR_32}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$DFL_REG32}; // #{$DFL_REG32} regs per thread   MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u64.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u64  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u64     %bbcp32, %bbcp32, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u64    %bbcp64, [#{$TRACECTXPTR_64}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$DFL_REG64}; // #{$DFL_REG64} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u64.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u64  %bbaux64, %bbaux64, 8;  //  "
    fileptr.puts "\tadd.u64     %bbcp64, %bbcp64, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u64    %bbcpfp, [#{$TRACECTXPTR_FP}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$DFL_REGFP}; // #{$DFL_REGFP} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u64.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u64  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u64     %bbcpfp, %bbcpfp, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u64    %smcpfp, [#{$SHMPTR_FP}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHMFP_pos}; // #{$SHMFP_pos} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u64.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u64  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u64     %smcpfp, %smcpfp, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u64    %smcp32, [#{$SHMPTR_32}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHM32_pos}; // #{$SHM32_pos} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u64.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u64  %bbaux64, %bbaux64, 4;  //  "
    fileptr.puts "\tadd.u64     %smcp32, %smcp32, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u64    %smcp64, [#{$SHMPTR_64}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHM64_pos}; // #{$SHM64_pos} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u64.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u64  %bbaux64, %bbaux64, 8;  //  "
    fileptr.puts "\tadd.u64     %smcp64, %smcp64, %bbaux64; //  "

    fileptr.puts "\n\tld.param.u64    %smcpaddr, [#{$SHMPTR_ADDR}];"
    fileptr.puts "\tmul.lo.u32  %bbaux, %bboff, #{$SHM_addr}; // #{$SHM_addr} regs per thread  MUST MATCH VALUE IN gpgpu_threads.h"
    fileptr.puts "\tcvt.u64.s32     %bbaux64, %bbaux;       //  "
    fileptr.puts "\tmul.lo.u64  %bbaux64, %bbaux64, 8;  //  "
    fileptr.puts "\tadd.u64     %smcpaddr, %smcpaddr, %bbaux64; //  "

    #Restore shared memory for all threads .. Do this because there is no guarantee that the shared memory is going to stay consistent, 
    #since blocks might be split based on how many threads are in Timing and how many in Rabbit, and there is no way to 
    #ensure that the same block goes to the same SM inside the physical GPGPU.
    fileptr.puts "\n// Restore Shared Context\n"
    poscount = 0
    while (poscount < $SHM_addr)
      fileptr.puts "#{$shmemmap["#{poscount}"]}\n"
      poscount += 1
    end
    fileptr.puts "// **********************\n\n"



    # Add instructions to enable pausing execution.
    fileptr.puts "\n\t// Checking if execution is to be paused and jumping"
    fileptr.puts "\tld.global.u64   %bbaux64, [%bbtp+32];"
    fileptr.puts "\tsetp.eq.u64 %p0_bbt, %bbaux64, 1;"
    fileptr.puts "\t@%p0_bbt bra #{$lastLabel};"

    # Add instructions to enable pausing execution.
    fileptr.puts "\n\t// Checking if thread has completed execution"
    fileptr.puts "\tld.global.u64   %bbaux64, [%bbtp+0];"
    fileptr.puts "\tsetp.eq.u64 %p0_bbt, %bbaux64, 0;"
    fileptr.puts "\t@%p0_bbt bra #{$lastLabel};"


    # Load skip instructions.
    fileptr.puts "\n\t// Checking if native fastforwarding is enabled"
    fileptr.puts "\tld.global.s64   %skipinstcount, [%bbtp+16];"
    fileptr.puts "\tsetp.gt.s64 %pinst_skip, %skipinstcount, 0;"
    fileptr.puts "\tmov.s64   %instskipcount, 0;"

    #temp = 0
    #fileptr.puts"\n//Pause execution if needed"
    #fileptr.puts "\tsetp.eq.u64 %p#{temp}_bbt, %bbaux64, #{temp};"
    #fileptr.puts "\t@%p#{temp}_bbt bra #{$lastLabel};"
    #fileptr.puts "\n"


    # Load instructions Branch and Taken Branch count.
    fileptr.puts "\n\t// Loading the branch count"
    fileptr.puts "\tld.global.u32   %scratchreg1, [%bbtp+80]; //CHECKME"

    fileptr.puts "\n\t// Loading the taken branch count"
    fileptr.puts "\tld.global.u32   %scratchreg2, [%bbtp+88]; //CHECKME"


    #
    fileptr.puts "\n\t// loading BB identifier and jumping"
    fileptr.puts "\tld.global.u64   %bbaux64, [%bbtp+0];"

    fileptr.puts "\n\t// saving current BB identifier and jumping"
    fileptr.puts "\tst.global.u64   [%bbtp+8], %bbaux64;"

    totalbb = @kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs


    # 		temp = 1
    temp = @kernel_FIRSTBB
    while (temp <= totalbb)
      fileptr.puts "\tsetp.eq.u64 %p#{temp}_bbt, %bbaux64, #{temp};"
      temp = temp+1
    end
    fileptr.puts "\n\n"
    #     temp = 1
    temp = @kernel_FIRSTBB
    while (temp <= totalbb)
      label = @kernel_bb["#{temp}"].getBBlabel
      fileptr.puts "\t@%p#{temp}_bbt bra #{label};"
      temp = temp+1
    end
  end

  fileptr.puts "\n// BBT startup code end\n\n"
end


def Insert_BB_begin_code(fileptr, bbid)
  singleinst = false
  lineend = @kernel_bb["#{bbid}"].getEnd
  #puts "#####"
  #puts bbid
  #puts lineend
  #puts "#####"
  if ((lineend == @kernel_bb["#{bbid}"].getStart) && /bra|bar/.match($instDecode["#{lineend}"].getOpcode))
    singleinst = true
  end


  fileptr.puts "\n\n// BBT set BB input begin\n"

  inarray = @kernel_bb["#{bbid}"].getBBPreds()
  if (inarray.size > 0)
    fileptr.puts "// Restore previously computed predicates"
    temp = 0

    while (temp < inarray.size())
      reg=inarray[temp].to_s().delete "%\""
      reg=reg.delete! "@!"
      #puts reg
      #puts "#{$predicates["#{reg}"]}"
      fileptr.puts "\tld.global.u32   %bbaux, [%bbtp+#{$predicates["#{reg}"]}];"
      fileptr.puts "\tsetp.eq.u32     %#{reg}, %bbaux,1;"
      temp = temp+1
    end
    fileptr.puts "// ***********************************\n"
  end

  fileptr.puts "//Here #{@kernel_bb["#{bbid}"].getStart} to  #{lineend} \n"
  if (!singleinst)
    temp = 0
    inarray = @kernel_bb["#{bbid}"].getLI.to_a()
    inarray.sort!
    while (temp < inarray.size())

      reg=inarray[temp].to_s().delete "%\""
      if ($programconstants["#{reg}"] == nil)
        if /f/.match(inarray[temp].to_s())
          if /d/.match(inarray[temp].to_s()) then
            #F64
            fileptr.puts "\tld.global.f64   %#{inarray[temp].to_s()}, [%bbcpfp64+#{@kernel_addressmap["#{reg}"]*8}];"
          else
            #F32
            fileptr.puts "\tld.global.f32   %#{inarray[temp].to_s()}, [%bbcpfp+#{@kernel_addressmap["#{reg}"]*4}];"
          end
        elsif /d/.match(inarray[temp].to_s())
          fileptr.puts "\tld.global.u64   %#{inarray[temp].to_s()}, [%bbcp64+#{@kernel_addressmap["#{reg}"]*8}];"
        elsif /h/.match(inarray[temp].to_s())
          fileptr.puts "\tld.global.u16   %#{inarray[temp].to_s()}, [%bbcp32+#{@kernel_addressmap["#{reg}"]*4}];"
        elsif /r/.match(inarray[temp].to_s())
          fileptr.puts "\tld.global.u32   %#{inarray[temp].to_s()}, [%bbcp32+#{@kernel_addressmap["#{reg}"]*4}];"
        end
      end

      temp = temp+1
    end

    fileptr.puts "// BBT set BB input end\n\n"
#----------------->NOV 4 new changes -------------------
#    if @kernel_bb["#{bbid}"].isSHMused
#      fileptr.puts "// Restore Shared Context\n"
#      poscount = 0
#      while (poscount < $SHM_addr)
#        fileptr.puts "#{$shmemmap["#{poscount}"]}\n"
#        poscount += 1
#      end
#      fileptr.puts "// **********************\n\n"
#    end
#-------------------------------------------------------
    fileptr.puts "\n$Lt_bbt_#{bbid-1}_inside:\n"

    #fileptr.puts "\n// Load updated SkipInstCount\n"
    #if ($M32)
    #  fileptr.puts "\tld.global.s32   %skipinstcount, [%bbtp+8];\n"
    #  fileptr.puts "\tld.global.s32   %instskipcount, [%bbtp+36];\n"
    #else
    #  fileptr.puts "\tld.global.s64   %skipinstcount, [%bbtp+16];\n"
    #  fileptr.puts "\tld.global.s64   %instskipcount, [%bbtp+72];\n"
    #end

    fileptr.puts "\n// Store Current BBID\n"
    if ($M32)
      fileptr.puts "\tst.global.u32   [%bbtp+4], #{bbid};\n"
    else
      fileptr.puts "\tst.global.u64   [%bbtp+8], #{bbid};\n"
    end
    fileptr.puts "// **********************\n\n"

  else
    fileptr.puts "// BBT set BB input end\n\n"
    #    fileptr.puts "\tld.global.s64   %skipinstcount, [%bbtp+16];\n"
    fileptr.puts "\n$Lt_bbt_#{bbid-1}_inside:\n"
    #fileptr.puts "\n// Load updated SkipInstCount\n"
    #if ($M32)
    #  fileptr.puts "\tld.global.s32   %skipinstcount, [%bbtp+8];\n"
    #  fileptr.puts "\tld.global.s32   %instskipcount, [%bbtp+36];\n"
    #else
    #  fileptr.puts "\tld.global.s64   %skipinstcount, [%bbtp+16];\n"
    #  fileptr.puts "\tld.global.s64   %instskipcount, [%bbtp+72];\n"
    #end
    fileptr.puts "\n// Store Current BBID\n"
    if ($M32)
      fileptr.puts "\tst.global.u32   [%bbtp+4], #{bbid};\n"
    else
      fileptr.puts "\tst.global.u64   [%bbtp+8], #{bbid};\n"
    end
    fileptr.puts "// **********************\n\n"


  end
end

def Insert_load_code(fileptr,lineNumber)
  #if (/const/.match(@kernel_instDecode["#{lineNumber}"].getOpcode))
  #  #load from a param.
  #  fileptr.puts "// BBT ld-const begin"
  #
  #
  #
  #elsif !(/__cuda/.match(@kernel_instDecode["#{lineNumber}"].getSrc1))

  if !(/__cuda/.match(@kernel_instDecode["#{lineNumber}"].getSrc1))

    fileptr.puts "// BBT Save Load Address begin"

    if (@kernel_instDecode["#{lineNumber}"].getSrc1Off)

      if ($programconstants["#{@kernel_instDecode["#{lineNumber}"].getSrc1}"] == nil)
        if ($M32)
          if /d/.match(@kernel_instDecode["#{lineNumber}"].getSrc1) then
            fileptr.puts "\tadd.u32         %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1}, #{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
          else
            fileptr.puts "\tcvt.u32.u32    %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
            fileptr.puts "\tadd.u32    %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
          end
          fileptr.puts "\tst.global.u32   [%bbtp+#{$lastlocation}], %bbaux64;"
        else
          if /d/.match(@kernel_instDecode["#{lineNumber}"].getSrc1) then
            fileptr.puts "\tadd.u64         %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1}, #{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
          else
            fileptr.puts "\tcvt.u64.u32    %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
            fileptr.puts "\tadd.u64    %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
          end
          fileptr.puts "\tst.global.u64   [%bbtp+#{$lastlocation}], %bbaux64;"
        end
      else
        consttype = $programconstants["#{@kernel_instDecode["#{lineNumber}"].getSrc1}"]
        if ($M32)
          if /Constant/.match(consttype)
            fileptr.puts "\tmov.u32        %bbaux64, #{@kernel_instDecode["#{lineNumber}"].getSrc1};"
            fileptr.puts "\tadd.u32        %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
          elsif /texture/.match(consttype)
            fileptr.puts "\tmov.u32        %bbaux64, #{@kernel_instDecode["#{lineNumber}"].getSrc1};"
            fileptr.puts "\tadd.u32        %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
          end
          fileptr.puts "\tst.global.u32   [%bbtp+#{$lastlocation}], %bbaux64;"
        elsif
          if /Constant/.match(consttype)
            fileptr.puts "\tmov.u64        %bbaux64, #{@kernel_instDecode["#{lineNumber}"].getSrc1};"
            fileptr.puts "\tadd.u64        %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
          elsif /texture/.match(consttype)
            fileptr.puts "\tmov.u64        %bbaux64, #{@kernel_instDecode["#{lineNumber}"].getSrc1};"
            fileptr.puts "\tadd.u64        %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
          end
          fileptr.puts "\tst.global.u64   [%bbtp+#{$lastlocation}], %bbaux64;"
        end

        #if $programconstants["#{@kernel_instDecode["#{lineNumber}"].getSrc1}"]
        #end
        #fileptr.puts "\tcvt.u32.u32    %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1}; //Storing address of constant variable in memory"
        #fileptr.puts "\tadd.u32    %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
        #fileptr.puts "\t//?????????????????????;"
      end


    else #Else if there is no offset

      if ($programconstants["#{@kernel_instDecode["#{lineNumber}"].getSrc1}"] == nil)

        if ($M32)
          if /d/.match(@kernel_instDecode["#{lineNumber}"].getSrc1) then
            fileptr.puts "\tst.global.u32   [%bbtp+#{$lastlocation}], %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
          else
            fileptr.puts "\tcvt.u32.u32    %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
            fileptr.puts "\tst.global.u32   [%bbtp+#{$lastlocation}], %bbaux64;"
          end
          # fileptr.puts "\tadd.u64         %bbaux64, %#{$instDecode["#{lineNumber}"].getSrc1}, 0};"
        else
          if /d/.match(@kernel_instDecode["#{lineNumber}"].getSrc1) then
            fileptr.puts "\tst.global,u64   [%bbtp+#{$lastlocation}], %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
          else
            fileptr.puts "\tcvt.u64.u32    %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
            fileptr.puts "\tst.global.u64   [%bbtp+#{$lastlocation}], %bbaux64;"
          end
          # fileptr.puts "\tadd.u64         %bbaux64, %#{$instDecode["#{lineNumber}"].getSrc1}, 0};"
        end
      else
        consttype = $programconstants["#{@kernel_instDecode["#{lineNumber}"].getSrc1}"]
        if ($M32)
          if /Constant/.match(consttype)
            fileptr.puts "\tmov.u32        %bbaux64, #{@kernel_instDecode["#{lineNumber}"].getSrc1};"
          elsif /texture/.match(consttype)
            fileptr.puts "\tmov.u32        %bbaux64, #{@kernel_instDecode["#{lineNumber}"].getSrc1};"
          end
          fileptr.puts "\tst.global.u32   [%bbtp+#{$lastlocation}], %bbaux64;"
        elsif
          if /Constant/.match(consttype)
            fileptr.puts "\tmov.u64        %bbaux64, #{@kernel_instDecode["#{lineNumber}"].getSrc1};"
          elsif /texture/.match(consttype)
            fileptr.puts "\tmov.u64        %bbaux64, #{@kernel_instDecode["#{lineNumber}"].getSrc1};"
          end
          fileptr.puts "\tst.global.u64   [%bbtp+#{$lastlocation}], %bbaux64;"
        end

      end

    end
    fileptr.puts "// BBT Save Load Address End"

    if ($M32)
      $lastlocation = $lastlocation+4
    else
      $lastlocation = $lastlocation+8
    end
  end
end


def Insert_predicate_retain_code(fileptr,predicate)
  fileptr.puts "//%%%%%% Store Predicate %%%%%%%%%"
  fileptr.puts "\t@%#{predicate} st.global.u32   [%bbtp+#{$predicates["#{predicate}"]}], 1;"
  fileptr.puts "\t@!%#{predicate} st.global.u32   [%bbtp+#{$predicates["#{predicate}"]}], 0;"
  fileptr.puts "//%%%%%% %%%%%%%%%%%%%% %%%%%%%%%\n"
end

def Insert_store_code(fileptr,lineNumber)

  if ! /__cuda/.match(@kernel_instDecode["#{lineNumber}"].getSrc1)
    fileptr.puts "// BBT Save Store Address begin"
    if /d/.match(@kernel_instDecode["#{lineNumber}"].getSrc1) then
      if ($M32)
        fileptr.puts "\tst.global.u32   [%bbtp+#{$lastlocation}], %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
      else
        fileptr.puts "\tst.global.u64   [%bbtp+#{$lastlocation}], %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
      end
    else
      if ($M32)
        fileptr.puts "\tcvt.u32.u32    %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
        fileptr.puts "\tadd.u32        %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
        fileptr.puts "\tst.global.u32  [%bbtp+#{$lastlocation}], %bbaux64;"
      else
        fileptr.puts "\tcvt.u64.u32    %bbaux64, %#{@kernel_instDecode["#{lineNumber}"].getSrc1};"
        fileptr.puts "\tadd.u64        %bbaux64,%bbaux64,#{@kernel_instDecode["#{lineNumber}"].getSrc1Off};"
        fileptr.puts "\tst.global.u64   [%bbtp+#{$lastlocation}], %bbaux64;"
      end

    end
    fileptr.puts "// BBT Save Store Address end"

    if /shared/.match(@kernel_instDecode["#{lineNumber}"].getOpcode)
      fileptr.puts @kernel_instDecode["#{lineNumber}"].getSharedStoreCode
    end

    if ($M32)
      $lastlocation = $lastlocation+4
    else
      $lastlocation = $lastlocation+8
    end
  end

end

def Insert_BB_end_code(fileptr, bbid)

  singleinst = false
  unibranch = false
  lineend = @kernel_bb["#{bbid}"].getEnd

  insert_branch_signature = false 
  op = @kernel_instDecode["#{lineend}"].getOpcode
  if /bra/.match(op) then
    insert_branch_signature = true
  end

  if !$termblocks.include?(bbid)
    op = @kernel_instDecode["#{lineend}"].getOpcode
    pred = @kernel_instDecode["#{lineend}"].getPred

    if ((@kernel_bb["#{bbid}"].getEnd == @kernel_bb["#{bbid}"].getStart) && /bra|bar/.match(op))
      singleinst = true
    end

    if (singleinst && (/bra.uni/.match(op)))
      unibranch = true
    end

    linestart = lineend
    if (!(@kernel_instDecode["#{linestart}"].getLabel) && !(/bra/.match(@kernel_instDecode["#{linestart}"].getOpcode)) && !(/bar/.match(@kernel_instDecode["#{linestart}"].getOpcode)) && !(/.loc/.match(@kernel_instDecode["#{linestart}"].getInst)) && !(/\}/.match(@kernel_instDecode["#{linestart}"].getInst)))
      #puts "HAHAHAHAHAHHAHAHAHAHAHHAHAHA********************* #{kernel_instDecode["#{linestart}"].getInst}"
      fileptr.puts "\t#{@kernel_instDecode["#{linestart}"].getInst}\n"
      if /ld/.match(@kernel_instDecode["#{linestart}"].getOpcode)
        Insert_load_code(fileptr,linestart)
      elsif /st/.match(@kernel_instDecode["#{linestart}"].getOpcode)
        Insert_store_code(fileptr,linestart)
      elsif /setp/.match(@kernel_instDecode["#{linestart}"].getOpcode)
        Insert_predicate_retain_code(fileptr, @kernel_instDecode["#{linestart}"].getDest())
      end

      InsertSkipInstCode(fileptr,bbid)
      fileptr.puts "\n\n// BBT set BB output begin\n"
      if @kernel_bb.has_key?("#{bbid+1}")
        destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
        if ($M32)
          fileptr.puts "\t#{pred} st.global.u32     [%bbtp+0], #{bbid+1}; // #{destlabel}\n"
          fileptr.puts "\t#{pred} st.global.u32     [%bbtp+12], 0; // Branch Not Taken\n"
        else
          fileptr.puts "\t#{pred} st.global.u64     [%bbtp+0], #{bbid+1}; // #{destlabel}\n"
          fileptr.puts "\t#{pred} st.global.u64     [%bbtp+24], 0; // Branch Not Taken\n"
        end
      end

    elsif (/bra/.match(op) && pred)
      destlabel =@kernel_instDecode["#{lineend}"].getDest
      destbbid = @kernel_labels["#{destlabel}"].getBBID
      InsertSkipInstCode(fileptr,bbid)
      fileptr.puts "\n\n// BBT set BB output begin\n"
      fileptr.puts "// Increment Branch Count\n"
      fileptr.puts "\tadd.u32 %scratchreg1, %scratchreg1, 1;\n"
      if ($M32) then
        fileptr.puts "\tst.global.u32 [%bbtp+40], %scratchreg1;\n"
      else
        fileptr.puts "\tst.global.u64 [%bbtp+80], %scratchreg1; //CHECKME\n"
      end

      #pred.delete!("!")
      if ($M32)
        fileptr.puts "\t#{pred} st.global.u32     [%bbtp+0], #{destbbid}; // #{destlabel}\n"
        fileptr.puts "\t#{pred} st.global.u32     [%bbtp+12], 1; // Branch Taken\n"
        fileptr.puts "// Increment Taken Branch Count\n"
        fileptr.puts "\t#{pred} add.u32 %scratchreg2, %scratchreg2, 1;\n"
        fileptr.puts "\t#{pred} st.global.u32 [%bbtp+44], %scratchreg2;\n"

      else
        fileptr.puts "\t#{pred} st.global.u64     [%bbtp+0], #{destbbid}; // #{destlabel}\n"
        fileptr.puts "\t#{pred} st.global.u64     [%bbtp+24], 1; // Branch Taken\n"
        fileptr.puts "// Increment Taken Branch Count\n"
        fileptr.puts "\t#{pred} add.u32 %scratchreg2, %scratchreg2, 1;\n"
        fileptr.puts "\t#{pred} st.global.u64 [%bbtp+88], %scratchreg2;//CHECKME\n"

      end
      destlabel = @kernel_bb["#{bbid+1}"].getBBlabel

      if /!/.match(pred) then
        notpred = pred.delete("!");
      else
        notpred = pred.insert(1,"!")
      end

      if ($M32)
        fileptr.puts "\t#{notpred} st.global.u32     [%bbtp+0], #{bbid+1}; // #{destlabel}\n"
        fileptr.puts "\t#{notpred} st.global.u32     [%bbtp+12], 0; // Branch not Taken\n"
      else
        fileptr.puts "\t#{notpred} st.global.u64     [%bbtp+0], #{bbid+1}; // #{destlabel}\n"
        fileptr.puts "\t#{notpred} st.global.u64     [%bbtp+24], 0; // Branch not Taken\n"
      end

    elsif (/bra/.match(op))
      if (singleinst)
        fileptr.puts "#{@kernel_instDecode["#{lineend}"].getInst}\n"
        fileptr.puts "\n\n// BBT set BB output begin\n"
        fileptr.puts "// Increment Branch Count\n"
        fileptr.puts "\tadd.u32 %scratchreg1, %scratchreg1, 1;\n"
        if ($M32) then
          fileptr.puts "\tst.global.u32 [%bbtp+40], %scratchreg1;\n"
        else
          fileptr.puts "\tst.global.u64 [%bbtp+80], %scratchreg1; //CHECKME\n"
        end


        fileptr.puts "// Increment Taken Branch Count\n"
        fileptr.puts "\tadd.u32 %scratchreg2, %scratchreg2, 1;\n"
        if ($M32) then
          fileptr.puts "\tst.global.u32 [%bbtp+44], %scratchreg2;\n"
        else
          fileptr.puts "\tst.global.u64 [%bbtp+88], %scratchreg2;//CHECKME\n"
        end

      else
        destlabel =@kernel_instDecode["#{lineend}"].getDest
        destbbid = @kernel_labels["#{destlabel}"].getBBID
        InsertSkipInstCode(fileptr,bbid)
        fileptr.puts "\n\n// BBT set BB output begin\n"
        fileptr.puts "// Increment Branch Count\n"
        fileptr.puts "\tadd.u32 %scratchreg1, %scratchreg1, 1;\n"
        if ($M32) then
          fileptr.puts "\tst.global.u32 [%bbtp+40], %scratchreg1;\n"
        else
          fileptr.puts "\tst.global.u64 [%bbtp+80], %scratchreg1; //CHECKME\n"
        end


        if ($M32)
          fileptr.puts "\tst.global.u32     [%bbtp+0], #{destbbid}; // #{destlabel}\n"
          fileptr.puts "\tst.global.u32     [%bbtp+12], 1; // Branch Taken\n"
          fileptr.puts "// Increment Taken Branch Count\n"
          fileptr.puts "\tadd.u32 %scratchreg2, %scratchreg2, 1;\n"
          fileptr.puts "\tst.global.u32 [%bbtp+44], %scratchreg2;\n"

        else
          fileptr.puts "\tst.global.u64     [%bbtp+0], #{destbbid}; // #{destlabel}\n"
          fileptr.puts "\tst.global.u64     [%bbtp+24], 1; // Branch Taken\n"
          fileptr.puts "// Increment Taken Branch Count\n"
          fileptr.puts "\tadd.u32 %scratchreg2, %scratchreg2, 1;\n"
          fileptr.puts "\tst.global.u64 [%bbtp+88], %scratchreg2;//CHECKME\n"

        end
      end


    elsif (pred != nil)
      destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
      InsertSkipInstCode(fileptr,bbid)
      fileptr.puts "\n\n// BBT set BB output begin\n"
      if ($M32)
        fileptr.puts "\t#{pred} st.global.u32     [%bbtp+0], #{bbid+1}; // #{destlabel}\n"
        fileptr.puts "\t#{pred} st.global.u32     [%bbtp+12], 0; // Branch Taken\n"
      else
        fileptr.puts "\t#{pred} st.global.u64     [%bbtp+0], #{bbid+1}; // #{destlabel}\n"
        fileptr.puts "\t#{pred} st.global.u64     [%bbtp+24], 0; // Branch Not Taken\n"
      end

    else
      if !/bar/.match(op)
        if !(@kernel_instDecode["#{lineend}"].getLabel)
          fileptr.puts "#{@kernel_instDecode["#{lineend}"].getInst}\n"
        end
      end
      InsertSkipInstCode(fileptr,bbid)
      fileptr.puts "\n\n// BBT set BB output begin\n"
      if @kernel_bb.has_key?("#{bbid+1}")
        destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
        if ($M32)
          fileptr.puts "\t#{pred} st.global.u32     [%bbtp+0], #{bbid+1}; // #{destlabel}\n"
          fileptr.puts "\t#{pred} st.global.u32     [%bbtp+12], 0; // Branch Not Taken\n"
        else
          fileptr.puts "\t#{pred} st.global.u64     [%bbtp+0], #{bbid+1}; // #{destlabel}\n"
          fileptr.puts "\t#{pred} st.global.u64     [%bbtp+24], 0; // Branch Not Taken\n"
        end
      end

    end

    if (!singleinst)
      temp = 0
      inarray = @kernel_bb["#{bbid}"].getLO.to_a()
      inarray.sort!
      while (temp < inarray.size())
        reg=inarray[temp].to_s().delete "%\""
        if ($programconstants["#{reg}"] == nil)
          if /f/.match(inarray[temp].to_s())
            if /d/.match(inarray[temp].to_s()) then
              fileptr.puts "\tst.global.f64   [%bbcpfp64+#{@kernel_addressmap["#{reg}"]*8}], %#{inarray[temp].to_s()};"
            else
              fileptr.puts "\tst.global.f32   [%bbcpfp+#{@kernel_addressmap["#{reg}"]*4}], %#{inarray[temp].to_s()};"
            end
          elsif /d/.match(inarray[temp].to_s())
            fileptr.puts "\tst.global.u64   [%bbcp64+#{@kernel_addressmap["#{reg}"]*8}], %#{inarray[temp].to_s()};"
          elsif /h/.match(inarray[temp].to_s())
            fileptr.puts "\tst.global.u16   [%bbcp32+#{@kernel_addressmap["#{reg}"]*4}], %#{inarray[temp].to_s()};"
          elsif /r/.match(inarray[temp].to_s())
            fileptr.puts "\tst.global.u32   [%bbcp32+#{@kernel_addressmap["#{reg}"]*4}], %#{inarray[temp].to_s()};"
          end
          #else
          #  fileptr.puts "\t #{reg} &&&&&&&&&&&& "
        end
        temp = temp+1
      end

    end

    if !(unibranch)
      #fileptr.puts "\t@%pskip_inst st.global.u64 [%bbtp+0], 0;\n"
      if !/bar/.match(op)
        fileptr.puts "\t@%pskip_inst exit;\n"
      else
# RETURN CONTROL AT BARRIER BLOCK
        fileptr.puts "// Return control at barrier block"
#        fileptr.puts "\t@%pskip_inst exit;\n"
        if ($M32)
          fileptr.puts "\t st.global.s32   [%bbtp+8], 0;//NOTEME"
          #fileptr.puts "\t add.s32 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"
          fileptr.puts "\t st.global.s32   [%bbtp+36], %instskipcount;"
        else
          fileptr.puts "\t st.global.s32   [%bbtp+16], 0;//NOTEME"#CHECKME: Shouldn't it be st.global.s64?
          #fileptr.puts "\t add.s64 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"
          fileptr.puts "\t st.global.s64   [%bbtp+72], %instskipcount;"
        end

        fileptr.puts "\texit;\n"
# RETURN CONTROL AT BARRIER BLOCK

      end



      lineend = @kernel_bb["#{bbid}"].getEnd
      #puts lineend

      op =@kernel_instDecode["#{lineend}"].getOpcode
      #puts op

      pred =@kernel_instDecode["#{lineend}"].getPred
      #puts pred



      if (/bra/.match(op) && pred)
        destlabel =@kernel_instDecode["#{lineend}"].getDest
        destbbid = @kernel_labels["#{destlabel}"].getBBID

        if /!/.match(pred)
          notpred = pred.delete("!")
          #puts notpred
        elsif
          notpred = pred.insert(1,"!")
          #puts notpred
        end
        fileptr.puts "\t#{notpred} bra $Lt_bbt_#{destbbid-1}_inside; // #{destlabel}\n"
        #puts "\t#{pred} bra $Lt_bbt_#{destbbid-1}_inside; // #{destlabel}\n"

        destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
        #puts destlabel

        #fileptr.puts "\t#{notpred} bra $Lt_bbt_#{bbid}_inside; // #{destlabel}\n"
        #puts "\t#{notpred} bra $Lt_bbt_#{bbid}_inside; // #{destlabel}\n"
        fileptr.puts "\t#{pred} bra $Lt_bbt_#{bbid}_inside; // #{destlabel}\n"
        #puts "\t#{pred} bra $Lt_bbt_#{bbid}_inside; // #{destlabel}\n"


      elsif (/bra/.match(op))
        destlabel =@kernel_instDecode["#{lineend}"].getDest
        destbbid = @kernel_labels["#{destlabel}"].getBBID
        fileptr.puts "\t#{pred} bra $Lt_bbt_#{destbbid-1}_inside; // #{destlabel}\n"
      elsif (pred != nil)
        destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
        fileptr.puts "\t#{pred} bra $Lt_bbt_#{bbid}_inside; // #{destlabel}\n"
      else
        if @kernel_bb.has_key?("#{bbid+1}")
          destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
          fileptr.puts "\t#{pred} bra $Lt_bbt_#{bbid}_inside; // #{destlabel}\n"
        end
      end

    end
    fileptr.puts "// BBT set BB output end\n\n"
    #exit(1)
  else
    fileptr.puts "//Included in termblocks!!"
    fileptr.puts "// BBT  Computation completed"
    if ($M32)
      fileptr.puts "\tst.global.u32   [%bbtp+0], 0;"
#*********************************************
      #Copied from InsertSkipInstCode
      fileptr.puts "\tsub.s32 %skipinstcount,%skipinstcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      fileptr.puts "\tsetp.lt.s32 %pskip_inst,%skipinstcount, 1;"

      fileptr.puts "\tst.global.s32   [%bbtp+8], 0;"
      #fileptr.puts "\t@%pskip_inst st.global.s32   [%bbtp+8], 0;"
      fileptr.puts "\tadd.s32 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      #fileptr.puts "\t@%pinst_skip add.s32 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      fileptr.puts "\tst.global.s32   [%bbtp+36], %instskipcount;"
      #fileptr.puts "\t@%pskip_inst st.global.s32   [%bbtp+36], %instskipcount;"
#*********************************************
    else
      fileptr.puts "\tst.global.u64   [%bbtp+0], 0;"
#*********************************************
      #Copied from InsertSkipInstCode
      fileptr.puts "\tsub.s64 %skipinstcount,%skipinstcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      fileptr.puts "\tsetp.lt.s64 %pskip_inst,%skipinstcount, 1;"

      #fileptr.puts "\t@%pskip_inst st.global.s32   [%bbtp+16], 0;"
      fileptr.puts "\tst.global.s32   [%bbtp+16], 0;"#CHECKME: Shouldn't it be st.global.s64?
      #fileptr.puts "\t@%pskip_inst st.global.s64   [%bbtp+72], %instskipcount;"
      fileptr.puts "\tst.global.s64   [%bbtp+72], %instskipcount;"

      #fileptr.puts "\t@!%pskip_inst add.s64 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      fileptr.puts "\tadd.s64 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"
#*********************************************
    end
    fileptr.puts "\texit;"
    fileptr.puts "#{@kernel_instDecode["#{lineend-1}"].getInst}\n"
    fileptr.puts "#{@kernel_instDecode["#{lineend}"].getInst}\n"
  end

end

def InsertSkipInstCode(fileptr,bbid)

  #Insert Redirection Instructions.
  lineend = @kernel_bb["#{bbid}"].getEnd
  if !$termblocks.include?(bbid)


    op = @kernel_instDecode["#{lineend}"].getOpcode
    pred = @kernel_instDecode["#{lineend}"].getPred


    #         fileptr.puts "\n\n// BBT set BB skip inst begin"

    #          fileptr.puts "\tld.global.u64 %bbaux64, [%bbtp+16];\n"
    #          fileptr.puts "//\tmov.u64 %bbaux64,0;\n"


    fileptr.puts "\n\t//Insert intruction to decrement the skip_inst count"

    if ($M32)
      #fileptr.puts "\tmov.u32 %skipinstcount,0;\n"
      fileptr.puts "\tsub.s32 %skipinstcount,%skipinstcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      fileptr.puts "\tsetp.lt.s32 %pskip_inst,%skipinstcount, 1;"

      fileptr.puts "\t@%pskip_inst st.global.s32   [%bbtp+8], 0;"
      #fileptr.puts "\t@!%pskip_inst add.s32 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      fileptr.puts "\t@%pinst_skip add.s32 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      fileptr.puts "\t@%pskip_inst st.global.s32   [%bbtp+36], %instskipcount;"

      #fileptr.puts "\t@!%pskip_inst st.global.s32   [%bbtp+8], %skipinstcount;"
      #fileptr.puts "\t@!%pskip_inst bra $Lt_bbt_#{bbid}_outside;\n"

    else
      #fileptr.puts "\tmov.u64 %skipinstcount,0;\n"
      fileptr.puts "\tsub.s64 %skipinstcount,%skipinstcount,#{@kernel_bb["#{bbid}"].getninst};\n"
      fileptr.puts "\tsetp.lt.s64 %pskip_inst,%skipinstcount, 1;"

      fileptr.puts "\t@%pskip_inst st.global.s32   [%bbtp+16], 0;"
      fileptr.puts "\t@%pskip_inst st.global.s64   [%bbtp+72], %instskipcount;"

      fileptr.puts "\t@!%pskip_inst add.s64 %instskipcount,%instskipcount,#{@kernel_bb["#{bbid}"].getninst};\n"


      #fileptr.puts "\t@!%pskip_inst st.global.s64   [%bbtp+16], %skipinstcount;"
      #fileptr.puts "\t@!%pskip_inst bra $Lt_bbt_#{bbid}_outside;\n"
    end

    if ((@kernel_bb["#{bbid}"].getEnd == @kernel_bb["#{bbid}"].getStart) && /bar/.match(op))
      fileptr.puts "\n//#{@kernel_instDecode["#{lineend}"].getInst}\n"
    end

    #         if (/bra/.match(op) && pred)
    # 						destlabel = @kernel_instDecode["#{lineend}"].getDest
    # 						destbbid = @kernel_labels["#{destlabel}"].getBBID
    # 						fileptr.puts "\t#{pred} bra $Lt_bbt_#{destbbid}_inside; // #{destlabel}\n"
    # 						destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
    #             notpred = pred.insert(1,"!")
    # 						fileptr.puts "\t#{notpred} bra $Lt_bbt_#{bbid+1}_inside; // #{destlabel}\n"
    # 				elsif (/bra/.match(op))
    # 						destlabel = @kernel_instDecode["#{lineend}"].getDest
    # 						destbbid = @kernel_labels["#{destlabel}"].getBBID
    # 						fileptr.puts "\t#{pred} bra $Lt_bbt_#{destbbid}_inside; // #{destlabel}\n"
    # 				elsif (pred != nil)
    # 						destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
    # 						fileptr.puts "\t#{pred} bra $Lt_bbt_#{bbid+1}_inside; // #{destlabel}\n"
    # 				else
    # 						if @kernel_bb.has_key?("#{bbid+1}")
    # 								destlabel = @kernel_bb["#{bbid+1}"].getBBlabel
    #                 fileptr.puts "\t#{pred} bra $Lt_bbt_#{bbid+1}_inside; // #{destlabel}\n"
    # 						end
    # 				end

    fileptr.puts "// BBT set BB skip inst end\n\n"
    fileptr.puts "\n\n$Lt_bbt_#{bbid-1}_outside:\n"


  end

end

def Insert_contaminated_kernel(outfileptr)

  local_counter_i = @kernel_FIRSTBB

  if ($M32)
    $BBTPOFFSET = 48
  else
    $BBTPOFFSET = 96
  end

  codestarts = false


  while (local_counter_i < @kernel_FIRSTBB+@kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs)
    if ($M32)
      $lastlocation = $BBTPOFFSET+4*$predicates.size()
      #########puts "BB : #{local_counter_i}"
      #########puts $lastlocation
    else
      $lastlocation = $BBTPOFFSET+8*$predicates.size()
    end

    linestart = @kernel_bb["#{local_counter_i}"].getStart
    lineend = @kernel_bb["#{local_counter_i}"].getEnd


    if (local_counter_i == @kernel_FIRSTBB)
      # Insert the Initialization code before the actual BB is implemented.
      while (linestart <= lineend-1)
        if @kernel_instDecode.has_key?("#{linestart}")
          while !(codestarts) && (linestart <= lineend-1)
            if !(@kernel_instDecode["#{linestart}"].getLabel)
              outfileptr.puts "\t#{@kernel_instDecode["#{linestart}"].getInst}\n"
            end
            if @kernel_instDecode["#{linestart}"].getLabel
              codestarts = true
              #outfileptr.puts "#{@kernel_instDecode["#{linestart}"].getInst}\n"
              Insert_BB_initialize_code(outfileptr)
              outfileptr.puts "#{@kernel_bb["1"].getBBlabel}:\n"
              Insert_BB_begin_code(outfileptr,local_counter_i)
            end
            linestart = linestart+1
          end

          if codestarts

            if !(@kernel_instDecode["#{linestart}"].getLabel)
              if !(/exit/.match(@kernel_instDecode["#{linestart}"].getOpcode))
                if /bra/.match(@kernel_instDecode["#{linestart}"].getOpcode)
                  #For the rare case where branch is at the begining of a code block. 
                  #Happens when a branch immediately follows a barrier. 
                  outfileptr.puts "//RC1: Increment Branch Count\n"
                  outfileptr.puts "\tadd.u32 %scratchreg1, %scratchreg1, 1;\n"
                  if ($M32) then
                    outfileptr.puts "\tst.global.u32 [%bbtp+40], %scratchreg1;\n"
                  else
                    outfileptr.puts "\tst.global.u64 [%bbtp+80], %scratchreg1; //CHECKME\n"
                  end


                  pred = @kernel_instDecode["#{linestart}"].getPred
                  if (pred) then
                    outfileptr.puts "\t// Increment Taken Branch Count\n"
                    outfileptr.puts "\t#{pred} add.u32 %scratchreg2, %scratchreg2, 1;\n"
                    if ($M32) then
                      outfileptr.puts "\t#{pred} st.global.u32 [%bbtp+44], %scratchreg2;\n"
                    else
                      outfileptr.puts "\t#{pred} st.global.u64 [%bbtp+88], %scratchreg2;//CHECKME\n"
                    end
                  else
                    outfileptr.puts "\t// Increment Taken Branch Count\n"
                    outfileptr.puts "\tadd.u32 %scratchreg2, %scratchreg2, 1;\n"
                    if ($M32) then
                      outfileptr.puts "\tst.global.u32 [%bbtp+44], %scratchreg2;\n"
                    else
                      outfileptr.puts "\t#{pred} st.global.u64 [%bbtp+88], %scratchreg2;//CHECKME\n"
                    end
                  end
                end

                outfileptr.puts "\t#{@kernel_instDecode["#{linestart}"].getInst}\n"
                if /ld/.match(@kernel_instDecode["#{linestart}"].getOpcode)
                  Insert_load_code(outfileptr,linestart)
                elsif /st/.match(@kernel_instDecode["#{linestart}"].getOpcode)
                  Insert_store_code(outfileptr,linestart)
                elsif /setp/.match(@kernel_instDecode["#{linestart}"].getOpcode)
                  Insert_predicate_retain_code(outfileptr, @kernel_instDecode["#{linestart}"].getDest())
                end
              end

            end


            linestart = linestart+1
          end
        end
      end

      Insert_BB_end_code(outfileptr,local_counter_i)
    else
      outfileptr.puts "#{@kernel_bb["#{local_counter_i}"].getBBlabel}:\n"
      outfileptr.puts "//Line\n"

      if (@kernel_bb["#{local_counter_i}"].getninst!= 0) then

        Insert_BB_begin_code(outfileptr,local_counter_i)
        while (linestart <= lineend-1)
          if !(@kernel_instDecode["#{linestart}"].getLabel)

            #outfileptr.puts "//ABCDEFGH\n"
            if !(/exit/.match(@kernel_instDecode["#{linestart}"].getOpcode))

              if /bra/.match(@kernel_instDecode["#{linestart}"].getOpcode)
                #For the rare case where branch is at the begining of a code block. 
                #Happens when a branch immediately follows a barrier. 
                outfileptr.puts "//RC1: Increment Branch Count\n"
                outfileptr.puts "\tadd.u32 %scratchreg1, %scratchreg1, 1;\n"
                if ($M32) then
                  outfileptr.puts "\tst.global.u32 [%bbtp+40], %scratchreg1;\n"
                else
                  outfileptr.puts "\tst.global.u64 [%bbtp+80], %scratchreg1; //CHECKME\n"
                end

                pred = @kernel_instDecode["#{linestart}"].getPred
                if (pred) then
                  outfileptr.puts "\t// Increment Taken Branch Count\n"
                  outfileptr.puts "\t#{pred} add.u32 %scratchreg2, %scratchreg2, 1;\n"
                  if ($M32) then
                    outfileptr.puts "\t#{pred} st.global.u32 [%bbtp+44], %scratchreg2;\n"
                  else
                    outfileptr.puts "\t#{pred} st.global.u64 [%bbtp+88], %scratchreg2;//CHECKME\n"
                  end

                else
                  outfileptr.puts "\t// Increment Taken Branch Count\n"
                  outfileptr.puts "\tadd.u32 %scratchreg2, %scratchreg2, 1;\n"
                  if ($M32) then
                    outfileptr.puts "\tst.global.u32 [%bbtp+44], %scratchreg2;\n"
                  else
                    outfileptr.puts "\t#{pred} st.global.u64 [%bbtp+88], %scratchreg2;//CHECKME\n"
                  end
                end
              end
              outfileptr.puts "\t#{@kernel_instDecode["#{linestart}"].getInst}\n"

              if /ld/.match(@kernel_instDecode["#{linestart}"].getOpcode)
                Insert_load_code(outfileptr,linestart)
              elsif /st/.match(@kernel_instDecode["#{linestart}"].getOpcode)
                Insert_store_code(outfileptr,linestart)
              elsif /setp/.match(@kernel_instDecode["#{linestart}"].getOpcode)
                Insert_predicate_retain_code(outfileptr, @kernel_instDecode["#{linestart}"].getDest())
              end
            end
          end
          linestart = linestart+1

        end

        Insert_BB_end_code(outfileptr,local_counter_i)
      else
        outfileptr.puts "$Lt_bbt_#{local_counter_i-1}_inside:\n"
        outfileptr.puts "$Lt_bbt_#{local_counter_i-1}_outside:\n"
        puts "kernel.rb    : CHECKME: No Instructions defined between $Lt_bbt_#{local_counter_i-1}_inside and $Lt_bbt_#{local_counter_i-1}_outside" 
      end
    end
    local_counter_i = local_counter_i+1
  end

end #End of Insert_contaminated_kernel



def decodetosesc(outfileptr)

  outfileptr.puts "\[KERNEL\]"
  outfileptr.puts "Name=#{@kernel_name}"
  outfileptr.puts "STARTPC=#{$PC}"
  outfileptr.puts "NUMBER_BB=#{@kernel_bb["1"].getTotalBBs}"

  outfileptr.puts "DFL_REG32= #{$BBCP32_pos}"
  outfileptr.puts "DFL_REG64= #{$BBCP64_pos}"
  outfileptr.puts "DFL_REGFP= #{$BBCPFP_pos}"
  outfileptr.puts "DFL_REGFP64= #{$BBCPFP64_pos}"

  outfileptr.puts "SM_REG32=#{$SHM32_pos}"
  outfileptr.puts "SM_REG64=#{$SHM64_pos}"
  outfileptr.puts "SM_REGFP=#{$SHMFP_pos}"
  outfileptr.puts "SM_REGFP64=#{$SHMFP64_pos}"
  outfileptr.puts "SM_ADRR=#{$SHM_addr}"
  outfileptr.puts "PREDICATES=#{$predicates.size()}"
  outfileptr.puts "TRACESIZE=#{@kernel_TRACESIZE+12+$predicates.size()}"


  temp_bb_string = String.new()
  dyn_inst = 0;
  temp_bb_string =""
  local_counter_i = @kernel_FIRSTBB

  while (local_counter_i < @kernel_FIRSTBB+@kernel_bb["#{@kernel_FIRSTBB}"].getTotalBBs)

    linestart = @kernel_bb["#{local_counter_i}"].getStart
    lineend = @kernel_bb["#{local_counter_i}"].getEnd


    outfileptr.puts "\n[BB]"
    outfileptr.puts "ID=#{local_counter_i}"
    
    div = @kernel_bb["#{@kernel_FIRSTBB + local_counter_i - 1}"].isDivergent
    if div
      outfileptr.puts "Divergent=1"
    else
      outfileptr.puts "Divergent=0"
    end

    rec = @kernel_bb["#{@kernel_FIRSTBB + local_counter_i - 1}"].getreconv_node
    if rec.size > 0
      outfileptr.puts "Reconv_BB=#{rec[0]}"
    else
      outfileptr.puts "Reconv_BB=0"
    end
    #outfileptr.puts "Reconv_BB=#{local_counter_i}"
    
    #outfileptr.puts "Number_insts=#{@kernel_bb["#{local_counter_i}"].getninst}"
    # outfileptr.puts "Label=#{@kernel_bb["#{local_counter_i}"].getBBlabel}"
    #outfileptr.puts "Number_insts=#{lineend-linestart+1}\n"
    
    barrier = 0

    while (linestart <= lineend)
      if @kernel_instDecode.has_key?("#{linestart}")
        if @kernel_instDecode["#{linestart}"].isInst
          # outfileptr.puts("0x#{$PC}:\t\t\t#{@kernel_instDecode["#{linestart}"].getInst}")
          op = @kernel_instDecode["#{linestart}"].getOpcode
          memaccesstype = 0 #register?
          changetoALU = false


          if /const/.match(op) then
            memaccesstype = 5
          elsif /tex/.match(op) then
            memaccesstype = 6
          elsif /shared/.match(op) then
            memaccesstype = 4
          elsif /local/.match(op)
            memaccesstype = 3
          elsif /param/.match(op)
            if /ld/.match(op) then
              changetoALU = true
              memaccesstype = 0
            else
              memaccesstype = 2
            end
          elsif /global/.match(op)
            memaccesstype = 1
          end

          #if /__cuda/.match(@kernel_instDecode["#{linestart}"].getSrc1) then
          #  if /LD/.match(op) then

          if /bar/.match(op) then
            barrier = 1
          end

          op = op.split(".")[0]

          kind = 0

          if /iBALU/.match($P2S["#{op}"].esesc_opcode) then
            kind = 3
          elsif /iLALU/.match($P2S["#{op}"].esesc_opcode)
            kind = 1
          elsif /iSALU/.match($P2S["#{op}"].esesc_opcode)
            kind = 2
          end



          if ($P2S["#{op}"].decompose == "1") then
            op1 = $P2S["#{op}"].esesc_opcode.split("+")[0]
            op2 = $P2S["#{op}"].esesc_opcode.split("+")[1]
            op1.lstrip!
            op1.rstrip!
            op2.lstrip!
            op2.rstrip!

            kind = 0 #FIXME : Assumption that compound instructions are usually arithmetic :-)

            #outfileptr.puts("#{$PC},#{op1},#{kind},#{$P2S["#{op}"].jumplabel},#{memaccesstype},#{@kernel_instDecode["#{linestart}"].getSrc1},#{@kernel_instDecode["#{linestart}"].getSrc2},,#{@kernel_instDecode["#{linestart}"].getDest}")
            temp_bb_string.insert(temp_bb_string.size(),("#{$PC},#{op1},#{kind},#{$P2S["#{op}"].jumplabel},#{memaccesstype},#{@kernel_instDecode["#{linestart}"].getSrc1},#{@kernel_instDecode["#{linestart}"].getSrc2},,#{@kernel_instDecode["#{linestart}"].getDest}\n"))
            dyn_inst = dyn_inst+1
            $PC += 4
            #outfileptr.puts("#{$PC},#{op2},#{kind},#{$P2S["#{op}"].jumplabel},#{memaccesstype},#{@kernel_instDecode["#{linestart}"].getDest},#{@kernel_instDecode["#{linestart}"].getSrc3},,#{@kernel_instDecode["#{linestart}"].getDest}")
            temp_bb_string.insert(temp_bb_string.size(),("#{$PC},#{op2},#{kind},#{$P2S["#{op}"].jumplabel},#{memaccesstype},#{@kernel_instDecode["#{linestart}"].getDest},#{@kernel_instDecode["#{linestart}"].getSrc3},,#{@kernel_instDecode["#{linestart}"].getDest}\n"))
            dyn_inst = dyn_inst+1
            $PC += 4
          else
            if (changetoALU == true) then
              kind = 0
              temp_bb_string.insert(temp_bb_string.size(),("#{$PC},iAALU,#{$P2S["#{op}"].jumplabel},#{memaccesstype},#{@kernel_instDecode["#{linestart}"].getSrc1},#{@kernel_instDecode["#{linestart}"].getSrc2},#{@kernel_instDecode["#{linestart}"].getSrc3},#{@kernel_instDecode["#{linestart}"].getDest}\n"))
            else
              #outfileptr.puts("#{$PC},#{$P2S["#{op}"].esesc_opcode},#{kind},#{$P2S["#{op}"].jumplabel},#{memaccesstype},#{@kernel_instDecode["#{linestart}"].getSrc1},#{@kernel_instDecode["#{linestart}"].getSrc2},#{@kernel_instDecode["#{linestart}"].getSrc3},#{@kernel_instDecode["#{linestart}"].getDest}")
              temp_bb_string.insert(temp_bb_string.size(),("#{$PC},#{$P2S["#{op}"].esesc_opcode},#{kind},#{$P2S["#{op}"].jumplabel},#{memaccesstype},#{@kernel_instDecode["#{linestart}"].getSrc1},#{@kernel_instDecode["#{linestart}"].getSrc2},#{@kernel_instDecode["#{linestart}"].getSrc3},#{@kernel_instDecode["#{linestart}"].getDest}\n"))
            end
            dyn_inst = dyn_inst+1
            $PC += 4
          end
        end
      end
      linestart += 1
    end

    #Increment the number of instructions and note the number of instructions
    #@kernel_bb["#{local_counter_i}"].add2ninst(dyn_inst)
    #outfileptr.puts "Number_insts=#{@kernel_bb["#{local_counter_i}"].getninst}"
    outfileptr.puts "Number_insts=#{dyn_inst}"

    #if (dyn_inst == 1) then
    if (barrier == 1) then
      outfileptr.puts "BARRIER=#{barrier}"
    elsif
      outfileptr.puts "BARRIER=0"
    end
    #outfileptr.puts "Number_insts=#{dyn_inst}"

    #Empty the string to the array
    temp_bb_string.each_line {|s| outfileptr.puts s}
    outfileptr.puts"[END_BB]\n"
    local_counter_i +=1
    temp_bb_string = ""
    dyn_inst = 0;

  end

  outfileptr.puts "\n[END_KERNEL]\n\n"
end

end #End of class
