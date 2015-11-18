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

$labels = Hash.new                                                # KEY: LABEL NAME, VALUE:  LINE_NUMBER and NEW LABEL NAME

class Label

  @lineNumber
  @NewLabel
  @BBID

  def initialize(lineNumber,line)
    @lineNumber = lineNumber
    @NewLabel = line
  end

  def getLineNumber
    return @lineNumber
  end

  def setBBID(bbid)
    @BBID = bbid
  end

  def getBBID
    return @BBID
  end

  def setAlias(newname)
    @NewLabel=newname
  end

  def getAlias(newname)
    return @NewLabel
  end


end




class BasicBlock


  # Local variables unique to each basic block.
  @BBID
  @line_Start
  @line_End
  @No_of_lines
  @BBlabel
  @No_of_Loads
  @No_of_Stores
  @Live_Ins
  @Live_Outs
  @Succ
  @Pre
  @termblock
  @ninst
  @isShMused
  @pred_in_use
  @reconv_node
  @isDiv
  @visited


  # Static variables, shared by all basic blocks inside a function.
  @@TotalBBs = 0
  @@ShMemAddr = 0
  @@ShM_u32var =  0
  @@ShM_u64var =  0
  @@ShM_f32var =  0


  def initialize(bbid, start, finish,lab,shMem,instcnt,preds_used)

    # Compute Live-Ins & Live Outs of previous basic blocks.
    @BBID = bbid
    @line_Start = start
    @line_End = finish
    @No_of_lines = finish-start+1
    @No_of_Stores = 0
    @No_of_Loads = 0
    @termblock = false
    @isShMused = shMem
    @ninst = instcnt
    @pred_in_use = preds_used
    #puts @BBID
    #puts @pred_in_use.inspect()
    #puts @line_Start
    #puts @line_End
    #puts @No_of_lines

    if (lab == nil)
      @BBlabel = "$Lt_bbt_#{@BBID-1}"
      $labels["$Lt_bbt_#{@BBID-1}"] = Label.new(@line_Start,@BBlabel)
      $labels["$Lt_bbt_#{@BBID-1}"].setBBID(@BBID)
    else
      @BBlabel = lab

      if ((@BBlabel != nil) && (lab != "headers"))
        $labels["#{@BBlabel}"].setBBID(@BBID)
      end
    end

    #puts @BBlabel

    if (lab != "headers") then
      incTotalBBs()
      @Pre = Array.new
      @Succ = Array.new
      @reconv_node = Array.new
      @isDiv = false
      @visited = false
      @Live_Ins = Set.new
      @Live_Outs = Set.new
    end

  end

  def resetstaticvariables
    @@TotalBBs = 0
    @@ShMemAddr = 0
    @@ShM_u32var =  0
    @@ShM_u64var =  0
    @@ShM_f32var =  0
  end

  def generateCFG(instDecode,labels,bb,firstBB)
    #puts "***************************"
    termblocks = Array.new
    temp = instDecode["#{@line_End}"].getDest
    if labels.has_key?("#{temp}")
      @Succ.insert(@Succ.size,labels["#{temp}"].getBBID)
      bb["#{labels["#{temp}"].getBBID}"].setPredec(@BBID)
    end

    #puts"<<<<<"
    #puts @BBID
    #puts @line_End
    #puts @line_Start
    #puts @@TotalBBs
    #puts ">>>>>"
    
    if ((@BBID < (firstBB+@@TotalBBs-1)) && (instDecode["#{@line_End}"].getOpcode != "bra.uni"))
      if labels.has_key?("$Lt_bbt_#{@BBID+1}")
        @Succ.insert(@Succ.size,@BBID+1)
        bb["#{@BBID+1}"].setPredec(@BBID)
      elsif labels.has_key?("#{bb["#{@BBID+1}"].getBBlabel}")
        if !@Succ.include?(@BBID+1)
          @Succ.insert(@Succ.size,@BBID+1)
        end
        bb["#{@BBID+1}"].setPredec(@BBID)
      end
    end

    if(instDecode["#{@line_End}"].getPred != nil)
      if (@BBID <= (firstBB+@@TotalBBs))
        @Succ.insert(@Succ.size,@BBID+1)
        bb["#{@BBID+1}"].setPredec(@BBID)
      end
    end

    @Succ.uniq!
    @Succ.sort!

    @Pre.uniq!
    @Pre.sort!

    if (@Succ.size>0)
      if (@Succ.size>1)
        @isDiv = true
      else
        @isDiv = false
        #@reconv_bb.insert(@reconv_bb.size,@Succ[0]);
      end
      #puts "BB #{@BBID} goes to BB #{@Succ.inspect}"
      #puts "BB #{@Pre.inspect} comes to BB #{@BBID}"
      #puts
    else
      @isDiv = false
      #@reconv_bb.insert(@reconv_bb.size,"0");
      #puts "BB #{@BBID} is the terminal BB"
      #puts "BB #{@Pre.inspect} comes to BB #{@BBID}"
      #puts
      @termblock = true;
      termblocks.insert(termblocks.size,@BBID)
    end

    return termblocks
  end

  def isSHMused
    return @isShMused
  end

  def getninst
    return @ninst
  end

  def add2ninst (instcount)
    @ninst = @ninst+instcount
  end

  def incTotalBBs
    @@TotalBBs += 1
  end

  def computeLiveVars
  end

  def getTotalBBs
    return @@TotalBBs
  end

  def getStart
    return @line_Start
  end

  def getEnd
    return @line_End
  end

  def getBBlabel
    return @BBlabel
  end

  def setPredec(bbid)
    @Pre.insert(@Pre.size,bbid)
  end

  def isDivergent
    return @isDiv
  end

  def getreconv_node  
    return @reconv_node
  end

  def setreconv_node(id)
    @reconv_node.insert(@reconv_node.size,id)
    @reconv_node.sort!
    @reconv_node.uniq!
  end

  def clearreconv_node
    @reconv_node.clear()
  end

  def markvisited
    @visited = true
  end

  def markunvisited
    @visited = false
  end

  def isvisited
    return @visited
  end



  def getPredec
    @Pre.uniq!
    return @Pre
  end

  def getSucc
    @Succ.uniq!
    return @Succ
  end

  def getLO
    return @Live_Outs
  end

  def setLO(lo)
    # @Live_Outs = @Live_Outs.
    if (lo != nil)
      @Live_Outs.clear
      lo.each {|item| @Live_Outs.add("#{item}")}
    end

  end

  def getBBPreds
    return @pred_in_use
  end

  def getLI
    return @Live_Ins
  end

  def setLI(li)
    if (li != nil)
      @Live_Ins.clear
      li.each {|item| @Live_Ins.add("#{item}")}
    end
  end

  def setLoadCount(temp)
    @No_of_Loads = temp
  end

  def setStoreCount(temp)
    @No_of_Stores = temp
  end

  def getInfo()
    tempstr = String.new()
    tempstr += "BBID=#{@BBID}\n"
    tempstr += "INST=#{@No_of_lines}\n"
    tempstr += "LOADS=#{@No_of_Loads}\n"
    tempstr += "STORES=#{@No_of_Stores}\n"
    tempstr += "SH MEM ACCESS=\ns"

    tempstr += "LIVEINS="
    @Live_Ins.each {|x| tempstr += "#{x}, " }
    tempstr += "\n"

    tempstr += "LIVEOUTS="
    @Live_Outs.each {|x| tempstr += "#{x}, " }
    tempstr += "\n"
    return tempstr
  end


  def isTermBlock
    return @termblock
  end
end
