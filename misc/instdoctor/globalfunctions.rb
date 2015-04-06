############################################################################
#    Copyright (C) 2010 by Alamelu Sankaranarayanan   #
#    alamelu@cse.ucsc.edu   #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License" , or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful" ,      #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not" , write to the                         #
#    Free Software Foundation" , Inc." ,                                      #
#    59 Temple Place - Suite 330" , Boston" , MA  02111-1307" , USA.             #
############################################################################


def Decode_line(line, linenumber)
  function_begins = false;
  line_string = line
  line.lstrip! and line = line.split(';')[0] and line = line.split(' ')[0]
  #   puts line

  if (line == "") then
    Add_to_PTX_hashmap("Blank Line",linenumber,line_string)

  elsif ($PTX_PERF_TUNING_DIRECTIVES.include?(line))
    #puts "#{line} -->PTX_PERF_TUNING_DIRECTIVES"
    Add_to_PTX_hashmap("PTX_PERF_TUNING_DIRECTIVES",linenumber,line_string)

  elsif ($PTX_VER_TARGET_DIRECTIVES.include?(line))
    #puts "#{line} -->PTX_VER_TARGET_DIRECTIVES"
    Add_to_PTX_hashmap("PTX_VER_TARGET_DIRECTIVES",linenumber,line_string)

  elsif ($PTX_DEBUGGING_DIRECTIVES.include?(line))
    #puts "#{line} -->PTX_DEBUGGING_DIRECTIVES"
    Add_to_PTX_hashmap("PTX_DEBUGGING_DIRECTIVES",linenumber,line_string)

  elsif ($PTX_LINKING_DIRECTIVES.include?(line))
    #puts "#{line} -->PTX_LINKING_DIRECTIVES"
    Add_to_PTX_hashmap("PTX_LINKING_DIRECTIVES",linenumber,line_string)

  elsif ($PTX_KERNELFUNC_DIRECTIVES.include?(line))
    #puts "%%%%%%%%%%%%%%%%%%%%%%%%% #{line}"
    function_begins = true; #Adding the function to the ptx hashmap will be done in the main function

  elsif /^\s*\/\/\s*/.match(line)
    Add_to_PTX_hashmap("Comment",linenumber,line_string)

  elsif ($PTX_DIRECTIVES.include?(line))
    #puts "#{line} -->PTX_DIRECTIVES"
    Add_to_PTX_hashmap("PTX_DIRECTIVES",linenumber,line_string)
  else
    puts "#{line} -->Unknown"
    puts "Unknown PTX file format. Error in line number #{linenumber}"
    exit(1)
  end

  return function_begins
end

def isConstanTexVar(line, linenumber)
  isConst = false

  if line == ""
    return isConst
  end

  line_string = line
  line.lstrip!
  line = line.split(';')[0]
  line = line.split(' ')[0]

  if  !(/^\s*\/\/\s*/.match(line))
    if ($PTX_CONST_TEX.include?(line)) then
      if /^\s*\.const\s*/.match(line)
        nomore = false
        position = 0
        size = 0
        while (!nomore)
          line = line_string.split(' ')[position]
          if (line == nil)
            nomore = true
          else
            if /\.b/.match(line)
              size = line_string.split('.b')[1]
              size = size.split(' ')[0]
            end
            position = position+1
          end
        end
        position = position-1
        #puts "Position #{position} = #{line_string.split(' ')[position]}"

        variable = line_string.split(' ')[position]
        if /\[/.match(variable)
          variable = variable.split('[')[0]
        end

        $programconstants["#{variable}"]="Constant+#{size}"

      elsif /^\s*.tex\s*/.match(line)
        puts "\n\n\n\n\nFound texture constant!!! #{line_string}\n\n\n\n"
        nomore = false
        position = 0

        while (!nomore)
          line = line_string.split(' ')[position]
          if (line == nil)
            nomore = true
          else
            position = position+1
          end
        end
        position = position-1
        #puts "Position #{position} = #{line_string.split(' ')[position]}"

        variable = line_string.split(' ')[position]
        if /\[/.match(variable)
          variable = variable.split('[')[0]
        end

        $programconstants["#{variable}"]="Texture"
      else
        puts "Weird... Shouldn't be here"
        exit(1)
      end
    end
  end
  return isConst
end


def Add_to_PTX_hashmap(linetype,linenumber,line_string)
  ptxline = PTXLineStruct.new
  ptxline.line_number = linenumber
  ptxline.line_type = linetype
  ptxline.line = line_string
  $PTX_hashmap.insert($PTX_hashmap.size, ptxline)
  #   $PTX_hashmap["#{linenumber}"] = ptxline
end

def Generate_contaminated_file(outfileptr)
  local_counter = 0
  #puts $PTX_hashmap.size


  while (local_counter < $PTX_hashmap.size)
    if (!($PTX_hashmap[local_counter].line_type == "Kernel") && !($PTX_hashmap[local_counter].line_type == "Function")) then
      outfileptr.puts $PTX_hashmap[local_counter].line
      #puts $PTX_hashmap[local_counter].line
    else
      #puts $PTX_hashmap[local_counter].line
      $PTX_hashmap[local_counter].line.write_to_file(outfileptr)
    end
    local_counter += 1
  end
end

def Generate_info_file(outfileptr)
  #Read the csv file with the input and generate the hashmap
  $p2sfile = File.open("#{$PTX_SESC_TRANS_CSV}")

  while $p2sfile.gets
    $_.rstrip!  and $_.lstrip!
    trans_inst =  PTX_sesc_Translation.new
    trans_inst.esesc_opcode = $_.split(",")[1]
    trans_inst.jumplabel = $_.split(",")[2]
    trans_inst.decompose = $_.split(",")[3]
    $P2S["#{$_.split(",")[0]}"] = trans_inst

  end
  $p2sfile.close

  if(DEBUG)
    $P2S.each{|transinst| puts transinst.inspect()}
  end

  local_counter = 0
  #   puts $PTX_hashmap.size
  while (local_counter < $PTX_hashmap.size)
    if (($PTX_hashmap[local_counter].line_type == "Kernel") && !($PTX_hashmap[local_counter].line_type == "Function")) then
      $PTX_hashmap[local_counter].line.decodetosesc(outfileptr)
    end
    local_counter += 1
  end

end
