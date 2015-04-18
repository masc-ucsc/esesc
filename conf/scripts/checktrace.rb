

last_pc   = 0
last_op   = 0
last_line = ""

while line_txt = gets do
  next unless line_txt =~ /^TR/

  line = line_txt.split(/\W+/)

  pc = line[1].to_i(16)

  op = line[5].to_i

  rd  = line[3][1..2].to_i
  rs1 = line[4][1..2].to_i
  rs2 = line[6][1..2].to_i

  if last_pc != 0 and (last_pc+4) != pc and (last_op<3 || last_op>9)
    puts "Missing PC: #{(last_pc+4).to_s(base=16)}"
    puts last_line
    puts line_txt
  end
  if last_op >=3 and last_op <=9 and (last_pc+4) == pc
    # last was branch with delay slot
    last_pc = 0
  else
    last_pc   = pc
  end

  last_op   = op
  last_line = line_txt
  
end

