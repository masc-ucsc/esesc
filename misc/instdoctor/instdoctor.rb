#!/usr/bin/env ruby
# A script that will pretend to resize a number of images
currentdir = Dir.pwd
Dir.chdir("/mada/users/alamelu/projs/esesc/misc/instdoctor")
%w{optparse globalfunctions globaldefinitions kernel instruction basicblock}.each{ |x| require x }

# This hash will hold all of the options
# parsed from the command-line by
# OptionParser.
options = {}

optparse = OptionParser.new do|opts|
  # Set a banner, displayed at the top
  # of the help screen.
  opts.banner = "Usage: optparse1.rb [options] file1 file2 ..."

  # Define the options, and what they do
  options[:verbose] = false
  opts.on( '-v', '--verbose', 'Output more information' ) do
    options[:verbose] = true
  end

  #   options[:quick] = false
  #   opts.on( '-q', '--quick', 'Perform the task quickly' ) do
  #     options[:quick] = true
  #   end

  #   options[:logfile] = nil
  #   opts.on( '-l', '--logfile FILE', 'Write log to FILE' ) do|file|
  #     options[:logfile] = file
  #   end

  options[:m32] = false
  opts.on( '--m32', 'Assume ptx generated for 32 bit architectures' ) do
    options[:m32] = true
    $M32 = true
  end

  options[:blocksizex] = 16
  opts.on( '-bx', '--blocksizex BLOCKSIZE.x', 'Specify the blocksize.x, default value is 16' ) do |bx|
    options[:blocksize] = bx
    $BLOCKSIZEX = bx
  end

  options[:blocksizey] = 16
  opts.on( '-by', '--blocksizey BLOCKSIZE.y', 'Specify the blocksize.y, default value is 16' ) do |by|
    options[:blocksize] = by
    $BLOCKSIZEY = by
  end

  options[:blocksizez] = 1
  opts.on( '-bz', '--blocksizez BLOCKSIZE.z', 'Specify the blocksize.z, default value is 1' ) do |bz|
    options[:blocksize] = bz
    $BLOCKSIZEZ = bz
  end

  options[:startpc] = 1073741824
  opts.on( '-pc', '--startpc STARTPC', 'Specify the starting PC, default value is 1073741824 (0x4000 0000)' ) do |pc|
    options[:startpc] = pc
    $PC = pc.to_i
  end


  # This displays the help screen, all programs are
  # assumed to have this option.
  opts.on( '-h', '--help', 'Display this screen' ) do
    puts opts
    exit
  end
end

# Parse the command-line. Remember there are two forms
# of the parse method. The 'parse' method simply parses
# ARGV, while the 'parse!' method parses ARGV and removes
# any options found there, as well as any parameters for
# the options. What's left is the list of files to resize.
if (ARGV.size() == 0)
  puts "Usage: instdoctor.rb [options] file1 file2 ..."
  puts "Type instdoctor.rb -h for a list of available options"
  exit
end

optparse.parse!

puts "Being verbose" if options[:verbose]
puts "Being quick" if options[:quick]
puts "Logging to file #{options[:logfile]}" if options[:logfile]

#puts "Blocksize.x #{options[:blocksizex]}"
#puts "Blocksize.x #{$BLOCKSIZEX}"

#puts "Blocksize.y #{options[:blocksizey]}"
#puts "Blocksize.y #{$BLOCKSIZEY}"

#puts "Blocksize.z #{options[:blocksizez]}"
#puts "Blocksize.z #{$BLOCKSIZEZ}"

puts "STARTPC #{$PC}"

puts "Compiling for 32 bit architectures" if options[:m32]


ARGV.each do|f|
  puts "Contaminating #{f}..."
  infilename = f.split(".ptx")[0]

  $filein = File.open("#{currentdir}/#{infilename}.ptx")                                          #INPUT FILE DESCRIPTOR
  puts "Opening #{infilename}.."

  $generatedptx = File.open("#{currentdir}/#{infilename}.contaminated.ptx", aModeString="w")      #OUTPUT FILE DESCRIPTOR
  $generatedptxinfo = File.open("#{currentdir}/#{infilename}.info", aModeString="w")              #OUTPUT FILE DESCRIPTOR
  #$generatedptxinfo = File.open("#{infilename}.info", aModeString="w")              #OUTPUT FILE DESCRIPTOR
  #puts "Generating #{$generatedptx} and #{$generatedptxinfo}"

  $number_of_lines = 1
  $number_of_kernels = 0
  $number_of_functions = 0
  puts "32 bit compilation is #{$M32}"

  if ($M32)
    $BBTPOFFSET=40
  else
    $BBTPOFFSET=80
  end



  while $filein.gets
    $_.rstrip!
    kernel_entry_point = Decode_line($_,$number_of_lines)

    if (kernel_entry_point) then
      if /.entry/.match($_) then
        if ($number_of_kernels > 0) then
          $Kernel_array[$number_of_kernels-1].analyze()
          Generate_contaminated_file($generatedptx)
          Generate_info_file($generatedptxinfo)
          $PTX_hashmap.clear()
        end

        puts "\ninstDoctor.rb: Reached kernel entrypoint"
        puts "instDoctor.rb: Kernel starts at #{$number_of_lines}"
        current_kernel = PTXKernel.new($_)
        #Add_to_PTX_hashmap("Kernel",$number_of_lines,current_kernel)
        Add_to_PTX_hashmap("Kernel",$PTX_hashmap.size(),current_kernel)
        puts "instDoctor.rb: Kernel ends at #{$number_of_lines}"

        $Kernel_array.insert($Kernel_array.size, current_kernel)
        $number_of_kernels += 1

        #     elsif /.function/.match($_) then
        #         puts "Function begins"
        #       function = Function.new()
        #       Add_to_PTX_hashmap("Function",$number_of_lines,function)
      end
    elsif (isConstanTexVar($_,$number_of_lines))
      puts "Const Found"
    else
      $number_of_lines += 1
    end

  end

  puts "\ninstDoctor.rb: There are #{$number_of_kernels} kernels"

  # Outline the basic blocks, and perform live variable analysis for each kernel
  #$Kernel_array.each{|current_kernel| current_kernel.analyze()}

  $Kernel_array[$number_of_kernels-1].analyze()

  # Generate the contaminated ptx.
  Generate_contaminated_file($generatedptx)
  Generate_info_file($generatedptxinfo)

  puts "instDoctor.rb: Exit\n\n"
  $filein.close

end



exit(0)
