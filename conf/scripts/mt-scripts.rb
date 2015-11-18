#!/usr/bin/env ruby

# This script is used for generating files to run multithreaded benchmarks
# through the launcher with eSESC.  You can set default values for some of the
# parameters by configuring enviornment variables in your .bashrc file
#
# By Ehsan K. Ardestani
#    Jose Renau

require 'fileutils'
require 'optparse'


### Set default options ####
options = Hash.new 
options[:benchmarks_a] =[]
options[:benchmarks_b] =[]
options[:email] = ENV['ESESC_SCRIPT_EMAIL']
options[:esescConf] = ENV['ESESC_SCRIPT_CONF'] 
options[:tpr] = 1 
options[:thermTT] = 0 # no throttling at all 
options[:maxSimedTime] = 90 # seconds 
options[:stackSize] = "unspecified" 
options[:esescExe] = ENV['ESESC_SCRIPT_ESESCEXE']

# Enable power is false by default unless environment variable is set
if(!ENV['ESESC_SCRIPT_DOPOWER'].nil?)
  options[:enablePower] = ENV['ESESC_SCRIPT_DOPOWER']
else
  options[:enablePower] = 0
end

if(!ENV['ESESC_SCRIPT_LABELDIR'].nil?)
  options[:labelDir] = ENV['ESESC_SCRIPT_LABELDIR']
end

if(!ENV['ESESC_SCRIPT_THERMMAIN'].nil?)
  options[:thermmain] = ENV['ESESC_SCRIPT_THERMMAIN']
end

if(options[:email].nil? && options[:esescConf].nil? && options[:specDir].nil? && options[:esescExe].nil? && options[:enablePower].nil?)
    puts "NOTE: you can set default values for some parameters using environment variables."
end

##############################
## Parse command line options
##############################
optparse = OptionParser.new do |opts|
  opts.banner = "Usage: mt-scripts.rb [options]"

  opts.on("-b bench1,bench2","--benchmarks bench1,bench2",Array,"specify list of benchmarks to run") do |bench|
    options[:benchmarks] = bench
  end

  opts.on("-e email","--email email","email address to send result to") do |email|
    options[:email] = email
  end

  opts.on("-m sampler","--sampler sampler","sampling mode to use") do |mode|
    options[:samplerMode] = mode
  end
  
  opts.on("-c conf_file","--conf conf_file","specify location of conf file to use") do |conf|
    options[:esescConf] = File.expand_path(conf)
  end

  opts.on("-d specDir","--specDir specDir","specify location of SPEC CPU benchmarks") do |dir|
    options[:specDir] = File.expand_path(dir)
  end   

  opts.on("-x esesc-exe","--exe esesc_exe","specify location of esesc executable") do |exe|
    options[:esescExe] = File.expand_path(exe)
  end

  opts.on("-v jobname","--extra","extra variable to append to the name of the job") do |jobname|
    options[:jobname] = "-" + jobname 
  end

  opts.on("-r TPr","--tpr TPr","thermal to performance interval ratio") do |tpr|
    options[:tpr] = tpr
  end

  opts.on("-t thermtt","--thermtt","set to 1 to enable thermal throttling for HP-CPU, 2 for LP-CPU") do |thermTT|
    options[:thermTT] = thermTT
  end

  opts.on("-a maxSimedTime","--maxSimedTime","max Simulated time") do |maxSimedTime|
    options[:maxSimedTime] = maxSimedTime
  end

  opts.on("-p enablePower","--enablePower","setting for whether power/thermal should be run") do |enablePower|
    options[:enablePower] = enablePower 
  end
  
  opts.on("-z labelDir","--labelFile","specify location of label files") do |labelDir|
    options[:labelDir] = labelDir
  end

  opts.on("-k thermmain","--thermmain","location of thermmain") do |thermmain|
    options[:thermmain] = File.expand_path(thermmain)
  end

  opts.on("-w powerSuffix","--powerSuffix","suffix for power files read by sesctherm") do |powerSuffix|
    options[:powerSuffix] = powerSuffix
  end

  opts.on("-s stackSize","--stack","set stack size used by QEMU") do |stackSize|
    options[:stackSize] = stackSize
  end

  opts.on("-q qemuExe","--qemu-exe","location of qemu executable for emulation only") do |qemuExe|
    options[:qemuExe] = File.expand_path(qemuExe)
  end
  
  opts.on("-g postfix","--post-fix","postfix to benchmark binaries") do |postfix|
    options[:postfix] = postfix
  end

  opts.on("-j inputPath","--inputPath","path to input files") do |inputPath|
    options[:inputPath] = File.expand_path(inputPath)
  end

  opts.on("-l","--local","To run the simulations locally") do |inputPath|
    options[:local] = true
  end

  opts.on("-u launcherPath","--launcherPath","Path to the launcher, including the launcher name") do |launcherPath|
    options[:launcherPath] = File.expand_path(launcherPath)
  end
  opts.on("-h","--help","Display this screen") do
    puts opts
    exit
  end

  if ARGV.empty?
    puts opts
    exit
  end
end
######################################

######################################
### Function to create script file ###
######################################
def writeScript(options)

  if(options[:esescConf]=="")
    esesc_conf_path = " -c ../esesc.conf"
  else
    esesc_conf_path = " -c " + options[:esescConf]
  end

  if options[:launcherPath]==nil
    benchExe = "../launcher"
  else
    benchExe = options[:launcherPath]
  end

  benchFullName_a = options[:benchName_a]
  txt=benchFullName_a.split(/[\.]+/)
  if txt.size > 1 # is it SPEC
    benchName_a = txt[1]
  else
    benchName_a = txt[0]
  end
  benchName = benchName_a 


  if (options[:numCores] > 1)
    benchFullName_b = options[:benchName_b]
    txt=benchFullName_b.split(/[\.]+/)
    if txt.size > 1 # is it SPEC
      benchName_b = txt[1]
    else
      benchName_b = txt[0]
    end
    benchName = benchName + "_" + benchName_b
  end

  if (options[:numCores] > 3)
    benchFullName_c = options[:benchName_c]
    txt=benchFullName_c.split(/[\.]+/)
    if txt.size > 1 # is it SPEC
      benchName_c = txt[1]
    else
      benchName_c = txt[0]
    end
    benchName = benchName + "_" + benchName_c
    benchFullName_d = options[:benchName_d]
    txt=benchFullName_d.split(/[\.]+/)
    if txt.size > 1 # is it SPEC
      benchName_d = txt[1]
    else
      benchName_d = txt[0]
    end
    benchName = benchName + "_" + benchName_d
  end

  if (options[:numCores] > 4)
    benchFullName_e = options[:benchName_e]
    txt=benchFullName_e.split(/[\.]+/)
    if txt.size > 1 # is it SPEC
      benchName_e = txt[1]
    else
      benchName_e = txt[0]
    end
    benchName = benchName + "_" + benchName_e
    benchFullName_f = options[:benchName_f]
    txt=benchFullName_f.split(/[\.]+/)
    if txt.size > 1 # is it SPEC
      benchName_f = txt[1]
    else
      benchName_f = txt[0]
    end
    benchName = benchName + "_" + benchName_f
    benchFullName_g = options[:benchName_g]
    txt=benchFullName_g.split(/[\.]+/)
    if txt.size > 1 # is it SPEC
      benchName_g = txt[1]
    else
      benchName_g = txt[0]
    end
    benchName = benchName + "_" + benchName_g
    benchFullName_h = options[:benchName_h]
    txt=benchFullName_h.split(/[\.]+/)
    if txt.size > 1 # is it SPEC
      benchName_h = txt[1]
    else
      benchName_h = txt[0]
    end
    benchName = benchName + "_" + benchName_h
  end
  benchName = benchName + "_" + options[:samplerMode]

  specDir = options[:specDir]
  if(specDir==nil)
    specDir = "/soe/eka/workspace/projects/benchmarks/oldsesc/rate/cpu2000-sparc/exe/" #default option
  end
  benchSampler = options[:samplerMode]

  stackSize = ""
  if(options[:stackSize].to_s != "unspecified" )
    stackSize = "-s " + options[:stackSize].to_s + " "
  end
 

    exeArgs = " " + options[:stdin] + " " + options[:arg_a]

    if (options[:numCores] > 1)
      exeArgs = exeArgs + " " + options[:arg_b]
    end
    if (options[:numCores] > 3)
      exeArgs = exeArgs + " " + options[:arg_c]
     if options[:arg_d] != "" 
      exeArgs = exeArgs + " " + options[:arg_d]
     end
    end
    if (options[:numCores] > 4)
      exeArgs = exeArgs + " " + options[:arg_e] + " " + options[:arg_f]
      exeArgs = exeArgs + " " + options[:arg_g] + " " + options[:arg_h]
    end

    benchSuffix = benchName

  ### Script to run eSESC ####
  benchDir = Dir.getwd + "/" + benchName
  if(benchSampler != 'qemu')
   scriptFile = File.new((benchName + "-run.sh"),"w")
  
    ## Commands for qsub
    #scriptFile.puts('#!/bin/sh ' + "\n" +
    #                '#$ -S /bin/sh' + "\n" +
    #                '#$ -m es' + "\n" +
    #                '#$ -M ' + options[:email] + "\n" +
    #                '#$ -cwd' + "\n" +
    #                '#$ -o ' + benchSuffix + ".log \n" +
    #                '#$ -e ' + benchSuffix + ".err \n" + 
    #                '#$ -N ' + benchSuffix + options[:jobname] + "\n\n")
    scriptFile.puts('#!/bin/sh ' + "\n" +
                    '#PBS -l nodes=1:ppn=2 -l walltime=240:00:00' + "\n" +
                    '#PBS -M ' + options[:email] + "\n")

    scriptFile.puts('if [ ! -d "' + benchDir + '" ]; then' + "\n" +
                    "  mkdir " + benchDir + "\n" +
                    "fi \n")
    scriptFile.puts("cd " + benchDir)
    
    #scriptFile.puts("cp -f " + options[:OOOxmlLoc] +"  .")
    inputpath = options[:inputPath]
    if(inputpath==nil)
      inputpath = "/mada/software/benchmarks/SPEC/input/"
    end
    scriptFile.puts("cp -rf " + File.join(inputpath, benchFullName_a , '/*') + ' .')
    if(options[:numCores] > 1)
      scriptFile.puts("cp -rf " + File.join(inputpath, benchFullName_b , '/*') + ' .')
    end
    if(options[:numCores] > 3)
      scriptFile.puts("cp -rf " + File.join(inputpath, benchFullName_c , '/*') + ' .')
      scriptFile.puts("cp -rf " + File.join(inputpath, benchFullName_d , '/*') + ' .')
    end
    if(options[:numCores] > 4)
      scriptFile.puts("cp -rf " + File.join(inputpath, benchFullName_e , '/*') + ' .')
      scriptFile.puts("cp -rf " + File.join(inputpath, benchFullName_f , '/*') + ' .')
      scriptFile.puts("cp -rf " + File.join(inputpath, benchFullName_g , '/*') + ' .')
      scriptFile.puts("cp -rf " + File.join(inputpath, benchFullName_h , '/*') + ' .')
    end
    #scriptFile.puts("export ESESC_numcores= " + options[:numCores].to_s())
    scriptFile.puts("export ESESC_ReportFile=\"" + benchSuffix + options[:jobname] + "\"")
    scriptFile.puts("export ESESC_samplerSel=\"" + options[:samplerMode] + "\"")

    if(stackSize.nil?)
      scriptFile.puts("export ESESC_BenchName=\"" + benchExe + exeArgs + "\"")
    else
      scriptFile.puts("export ESESC_BenchName=\"" + stackSize + benchExe + exeArgs + "\"")
    end
    if exeArgs =~ /facesim/
      scriptFile.puts("export ESESC_nInstSkip   = 1e9")
    end
    #puts("esescExe is" + options[:esescExe] + "\n")
    scriptFile.puts(options[:esescExe] + esesc_conf_path)
    scriptFile.puts("exit 0")
    scriptFile.chmod(0755)
    scriptFile.close
  
  ### Script for running with QEMU emulation mode only 
  else
    if(options[:qemuExe].nil?)
      puts "ERROR: qemu executable not specified"
      exit(1)
    end
    qemuRun = File.new((benchName + "-qemurun.sh"),"w")
    qemuRun.puts('#!/bin/sh ' + "\n" +
                 '#$ -S /bin/sh' + "\n" +
                 '#$ -m es' + "\n" +
                 '#$ -M ' + options[:email] + "\n" +
                 '#$ -cwd' + "\n" +
                 '#$ -o ' + benchSuffix + ".log \n" +
                 '#$ -e ' + benchSuffix + ".err \n" + 
                 '#$ -N ' + benchSuffix + options[:jobname] + "\n\n")
    qemuRun.puts('if [ ! -d "' + benchDir + '" ]; then' + "\n" +
                 "  mkdir " + benchDir + "\n" +
                 "fi \n")
    qemuRun.puts("cd " + benchDir)
    qemuRun.puts("cp -rf " + File.join(specDir , "input" , benchName , '/*') + ' .')
    qemuRun.puts("cp -f " + File.join(specDir , "exe" , benchExe ) + " .")    
    qemuRun.puts(options[:qemuExe] + " " + benchExe + exeArgs + " " + input)
    qemuRun.puts("exit 0")
    qemuRun.chmod(0755)
    qemuRun.close
  end
end
############################################
def getArg(ba,parsecN)
  arg =  case ba
         when "183.equake"     then nil
         when "403.gcc"        then "166.i -o 166.s"
         when "172.mgrid"      then nil
         when "179.art"        then "-scanfile c756hel.in -trainfile1 a10.img -trainfile2 hc.img -stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10"
         when "177.mesa"       then "-frames 1000 -meshfile mesa.in -ppmfile mesa.ppm"
         when "175.vpr"        then "net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2"
         when "181.mcf"        then "inp.in"
         when "254.gap"        then "-l ./ -q -m 192M" 
         when "173.applu"      then nil
         when "450.soplex"     then "-s1 -e -m45000 pds-50.mps"
         when "255.vortex"     then "lendian1.raw"
         when "300.twolf"      then "ref"
         when "168.wupwise"    then nil
         when "400.perlbench"  then "-I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1"  
         when "437.leslie3d"   then nil
         when "186.crafty"     then nil
         when "171.swim"       then nil
         when "410.bwaves"     then nil
         when "453.povray"     then "SPEC-benchmark-ref.ini" 
         when "433.milc"       then nil
         when "444.namd"       then "--input namd.input --iterations 38 --output namd.out"
         when "447.dealII"     then "23"
         when "482.sphinx3"    then "ctlfile . args.an4"
         when "462.libquantum" then "1397 8"
         when "470.lbm"        then "3000 reference.dat 0 0 100_100_130_ldc.of"
         when "473.astar"      then "BigLakes2048.cfg"
         when "blackscholes"   then (parsecN-1).to_s + " in_64K.txt out"  # 4 7
         when "fluidanimate"   then (parsecN).to_s + " 5 in_300K.fluid out.fluid" # 4 8
         when "swaptions"      then "-ns 64 -sm 20000 -nt " + (parsecN-1).to_s # 4 7
         when "x264"           then "--quiet --qp 20 --partitions b8x8,i4x4 --ref 5 --direct auto --b-pyramid --weightb --mixed-refs --no-fast-pskip --me umh --subme 7 --analyse b8x8,i4x4 --threads " + (parsecN-1).to_s + " -o eledream.264 eledream_640x360_128.y4m" # 3 7
         when "bodytrack"      then "sequenceB_4 4 4 4000 5 0 " + (parsecN - ((parsecN - 4)/4) -1).to_s # 3 6
         when "facesim"        then "-timing -threads " + (parsecN- ((parsecN - 4)/2)).to_s # 4 6
         when "ferret"         then "corel lsh queries 10 20 " + (parsecN/4).to_s + " output.txt" # 1 2
         when "dedup"          then "-c -p -f -t 4 -i media.dat -o output.dat.ddp"
         when "canneal"        then parsecN.to_s + " 15000 2000 400000.nets 128"
         when "ocean"          then "-p2 -n1026"
         when "fft"            then "-p4 -m22"
         when "radix"          then "-p4 -r1024 -n20000000"
         when "fmm"            then nil
         when "dummy"          then nil  
         else 
           puts "WARNING: '#{ba}' is not a valid benchmark"
           exit
         end
  return arg
end

def getStdin(ba)
  stdin =  case ba
           when "183.equake"    then "inp.in"
           when "403.gcc"       then nil
           when "172.mgrid"     then "mgrid.in"
           when "179.art"       then nil
           when "177.mesa"      then nil
           when "175.vpr"       then nil
           when "181.mcf"       then nil
           when "254.gap"       then "ref.in"
           when "173.applu"     then "applu.in"
           when "450.soplex"    then nil
           when "255.vortex"    then nil
           when "300.twolf"     then nil
           when "168.wupwise"   then nil
           when "400.perlbench" then nil
           when "437.leslie3d"  then "leslie3d.in"
           when "186.crafty"    then "crafty.in"
           when "171.swim"      then "swim.in"
           when "410.bwaves"    then nil
           when "453.povray"    then nil
           when "433.milc"      then "su3imp.in"
           when "444.namd"      then nil
           when "447.dealII"    then nil
           when "482.sphinx3"   then nil
           when "462.libquantum" then nil
           when "470.lbm"       then nil
           when "473.astar"     then nil
           when "blackscholes"  then nil
           when "fluidanimate"  then nil
           when "swaptions"     then nil
           when "x264"          then nil 
           when "bodytrack"     then nil 
           when "facesim"       then nil 
           when "ferret"        then nil 
           when "dedup"         then nil 
           when "canneal"       then nil 
           when "ocean"         then nil 
           when "fft"           then nil 
           when "radix"         then nil 
           when "fmm"           then "input.16384.p4"
           when "dummy"         then nil 
           end
  return stdin 
end

################################################################3
# Parse command line options
optparse.parse!

# Check that parameters are valid 
if(options[:jobname].nil?)
  options[:jobname] = ""
end

if(options[:benchmarks].empty?)
  puts "No benchmarks specified exiting"
  exit(1)
end

if(options[:samplerMode].nil?)
  puts "ERROR: No sampler mode specified"
  exit(1)
end

if((options[:esescConf])!=nil)
  if(File.exists?(options[:esescConf]))
    options[:esescConf] = File.expand_path(options[:esescConf]) 
  else
    puts "ERROR: configuration file '#{options[:esescConf]}' not found"
    exit(1)
  end
else
  options[:esescConf]=""
end

#if(options[:OOOxmlLoc]==nil)
#  options[:OOOxmlLoc] = ""
#end

puts "Generating scripts using the following parameters"
puts "-----------------------------------"
options.each_pair { |key,value| 
  if  value.nil?
    puts "ERROR: need to specify value for #{key}"
    exit(1)
  end
  if(!value.is_a?(Array))
    puts "paramter type is #{value.class}  parameter '#{key}' is: '#{value}'" 
  end
}
puts "-----------------------------------"
   
single_core = Array.[]("183.equake", "403.gcc", "172.mgrid","177.mesa","179.art", 
                       "175.vpr","181.mcf","254.gap",
                       "173.applu","450.soplex",
                       "300.twolf", "168.wupwise","400.perlbench","437.leslie3d", "410.bwaves",
                       "171.swim", "186.crafty", "453.povray", "433.milc", "255.vortex", 
                       "462.libquantum", "470.lbm", "473.astar", "447.dealII")

#single_core = Array.[]("183.equake", "403.gcc"  , "177.mesa"   , "447.dealII"   , "255.vortex"  , "186.crafty",
#                       "175.vpr"   , #"181.mcf", "254.gap"    , 
#                       "173.applu"    , "470.lbm"     , #"433.milc"  , 
#                       "172.mgrid", 
#                       "177.mesa"  , "300.twolf", "168.wupwise", "400.perlbench", "437.leslie3d", "444.namd"       ,
#                       "410.bwaves", #"171.swim" , "450.soplex" , 
#                       "453.povray"   , "179.art"     , #"462.libquantum",
#                       "473.astar"  )

#----------------------------->
dual_core_A = Array.[]("183.equake"  , "403.gcc"     , "177.mesa"  , "447.dealII" , 
                       "255.vortex"  , "186.crafty"  ,"175.vpr"    , "181.mcf"    , 
                       "254.gap"     , "173.applu"   , "470.lbm"   , "433.milc"   , 
                       "172.mgrid" )

dual_core_B = Array.[]("177.mesa"    , "300.twolf"  , "168.wupwise", "400.perlbench", 
                       "437.leslie3d", "444.namd"   , "410.bwaves" , "171.swim"   , 
                       "450.soplex"  , "453.povray" , "179.art"    , "462.libquantum",
                       "473.astar"  )

#----------------------------<
quad_core_A = Array.[]("183.equake"  , "177.mesa"   , "447.dealII"   , "255.vortex"  , 
                       "186.crafty"  , "171.swim"   , "181.mcf"      , "254.gap"     , 
                       "173.applu"   , "470.lbm"    , "433.milc"     , "437.leslie3d",
                       "172.mgrid" )

quad_core_B = Array.[]("177.mesa"    , "300.twolf"  , "168.wupwise" , "400.perlbench" , 
                       "444.namd"    , "453.povray" , "179.art"     , "462.libquantum",
                       "410.bwaves"  , "175.vpr"    , "450.soplex"  , "403.gcc"       , 
                       "473.astar"  )

quad_core_C = Array.[]("410.bwaves"  , "171.swim"   , "462.libquantum", "403.gcc"       ,
                       "453.povray"  ,"473.astar"   ,  "450.soplex"    ,"179.art"      ,  
                       "447.dealII"  , "300.twolf"  , "168.wupwise"   , "400.perlbench" ,  
                       "444.namd"       )
quad_core_D = Array.[]("447.dealII"  , "410.bwaves" , "450.soplex"    , "177.mesa" , 
                       "300.twolf"   , "403.gcc", "400.perlbench" , "444.namd" ,
                       "453.povray"  , "179.art"    , "462.libquantum", "168.wupwise"  ,
                       "473.astar"  )
#----------------------------<

octo_core_A = Array.[]("183.equake"          , "177.mesa"   , "447.dealII"   , "255.vortex"    ,
"186.crafty"           , "171.swim"   , "181.mcf"       , "254.gap"       ,
"173.applu"            , "470.lbm"    , "433.milc"      , "437.leslie3d"  ,
"172.mgrid")

octo_core_B = Array.[]("177.mesa"            , "300.twolf"  , "168.wupwise"   , "400.perlbench" ,
"444.namd"             , "453.povray" , "179.art"       , "462.libquantum",
"410.bwaves"           , "175.vpr"    , "450.soplex"    , "403.gcc"       ,
"473.astar"  )

octo_core_C = Array.[]("410.bwaves"          , "171.swim"   , "462.libquantum", "403.gcc"       ,
"453.povray"           , "473.astar"  , "450.soplex"    , "179.art"       ,
"447.dealII"          , "300.twolf"  , "168.wupwise"   , "400.perlbench" ,
"444.namd"       )

octo_core_D = Array.[]("447.dealII"         , "410.bwaves" , "450.soplex"    , "177.mesa"      ,
"300.twolf"            , "403.gcc"    , "400.perlbench" , "444.namd"      ,
"453.povray"           , "179.art"    , "462.libquantum", "168.wupwise"   ,
"300.twolf"  )

octo_core_E = Array.[]("177.mesa"            , "450.soplex"    , "410.bwaves", "447.dealII"   ,
"410.bwaves"            , "400.perlbench" , "403.gcc"   , "255.vortex"   ,
"255.vortex"           , "462.libquantum", "179.art"   , "181.mcf",
"473.astar"  )


octo_core_F = Array.[]("400.perlbench"       , "177.mesa"      , "300.twolf" , "168.wupwise",
"462.libquantum"       , "444.namd"      , "453.povray", "179.art"    ,
"403.gcc"              , "410.bwaves"    , "175.vpr"   , "450.soplex" ,
"175.vpr"  )

octo_core_G = Array.[]("462.libquantum"      , "410.bwaves" , "171.swim"      , "403.gcc"       ,
"450.soplex"           , "453.povray" , "473.astar"     , "181.mcf"       ,
"168.wupwise"          , "447.dealII", "300.twolf"     , "400.perlbench" ,
"444.namd")

octo_core_H = Array.[]("473.astar"          , "447.dealII", "450.soplex"    , "177.mesa"      ,
"447.dealII"              , "300.twolf"  , "400.perlbench" , "444.namd"      ,
"179.art"              , "453.povray" , "462.libquantum", "168.wupwise"   ,
"181.mcf"  )

#----------------------------<
#mix_octo_core_A = Array.[]\
#("447.dealII"                 , "255.vortex"   , "186.crafty"      , "171.swim"     ,
#"181.mcf"                     , "173.applu"    , "437.leslie3d"  )

mix_octo_core_A = Array.[]("168.wupwise"                , "400.perlbench", "444.namd"        , "453.povray"   ,
"186.crafty"                  , "450.soplex"   , "175.vpr")

mix_octo_core_B = Array.[]("181.mcf"                 , "171.swim"     , "179.art"  , "447.dealII"   ,
"453.povray"                   , "255.vortex", "473.astar"       )

#mix_octo_core_D = Array.[]\
#("450.soplex"                 , "177.mesa"     , "168.wupwise"     , "400.perlbench",
#"453.povray"                  , "179.art"      , "473.astar" )


mix_octo_core_C = Array.[]("blackscholes"               , "swaptions"    , "ocean"            , "bodytrack"    ,
"fluidanimate"                , "fft"      , "ferret")

mix_octo_core_D = Array.new(7,"dummy")
mix_octo_core_E = Array.new(7,"dummy")
mix_octo_core_F = Array.new(7,"dummy")
mix_octo_core_G = Array.new(7,"dummy")
mix_octo_core_H = Array.new(7,"dummy")

#----------------------------<
#parsec = Array.[]("blackscholes", "swaptions", "x264", "bodytrack", "fluidanimate", "facesim", "ferret", "canneal")
parsec = Array.[]("blackscholes", "swaptions", "x264", "bodytrack", "fluidanimate", "facesim", "ferret", "canneal", "ocean", "fft", "fmm", "radix")
#parsec = Array.[]("ocean", "fft", "fmm", "radix")

## Set benchmarks to run
options[:numCores] = case options[:benchmarks][0]
  when "single_core"    then 1
  when "dual_core"      then 2
  when "quad_core"      then 4
  when "octo_core"      then 8
  when "parsec_dual_core"      then 1
  when "parsec_quad_core"      then 1
  when "parsec_octo_core"      then 1
  when "parsec_16_core"      then 1                       
  when "mix_octo_core"      then 4
  else 1
end

options[:benchmarks_a] = case options[:benchmarks][0]
  when "single_core"     then single_core
  when "dual_core"       then dual_core_A
  when "quad_core"       then quad_core_A
  when "octo_core"       then octo_core_A
  when "parsec_dual_core"       then parsec
  when "parsec_quad_core"       then parsec
  when "parsec_octo_core"       then parsec
  when "parsec_16_core"       then parsec                           
  when "mix_octo_core"       then mix_octo_core_A
  else ""
end

options[:parsecN] = case options[:benchmarks][0]
  when "parsec_dual_core"       then 2
  when "parsec_quad_core"       then 4
  when "parsec_octo_core"       then 8
  when "parsec_16_core"         then 16
  end

if options[:benchmarks_a] == ""
  puts "Unrecognized benchmarks is specified"
  exit
end

options[:benchmarks_b] = case options[:benchmarks][0]
  when "dual_core"       then dual_core_B
  when "quad_core"       then quad_core_B
  when "octo_core"       then octo_core_B
  when "mix_octo_core"       then mix_octo_core_B
  else ""
end

options[:benchmarks_c] = case options[:benchmarks][0]
  when "quad_core"       then quad_core_C
  when "octo_core"       then octo_core_C
  when "mix_octo_core"       then mix_octo_core_C
  else ""
end

options[:benchmarks_d] = case options[:benchmarks][0]
  when "quad_core"       then quad_core_D
  when "octo_core"       then octo_core_D
  when "mix_octo_core"       then mix_octo_core_D
  else ""
end
options[:benchmarks_e] = case options[:benchmarks][0]
  when "octo_core"       then octo_core_E
  when "mix_octo_core"       then mix_octo_core_E
  else ""
end
options[:benchmarks_f] = case options[:benchmarks][0]
  when "octo_core"       then octo_core_F
  when "mix_octo_core"       then mix_octo_core_F
  else ""
end
options[:benchmarks_g] = case options[:benchmarks][0]
  when "octo_core"       then octo_core_G
  when "mix_octo_core"       then mix_octo_core_G
  else ""
end
options[:benchmarks_h] = case options[:benchmarks][0]
  when "octo_core"       then octo_core_H
  when "mix_octo_core"       then mix_octo_core_H
  else ""
end
## Create scripts

# there is at least one core 
# FIRST CORE
options[:benchmarks_a].each_index do |b|
  ba = options[:benchmarks_a][b]
  options[:benchName_a] = ba;


  arg_a = getArg(ba,options[:parsecN])
  stdin = getStdin(ba)

  txt=ba.split(/[\.]+/)
  if txt.size > 1  #is it SPEC? like 186.crafty
    ba = txt[1]
    if (ba == "gcc")
      ba = "gcc_06"
    end
  end
  

  if (arg_a != nil)
    options[:arg_a] = "-- " + ba + "  " + arg_a
  else
    options[:arg_a] = "-- " + ba
  end


#SECOND CORE
if (options[:numCores] > 1)

    bb = options[:benchmarks_b][b]
    options[:benchName_b] = bb;


    arg_b = getArg(bb,options[:parsecN])
    stdin_b = getStdin(bb)
    if (stdin != nil and stdin_b != nil)
      puts "Error! Only one standard input is allowed!" + " " + ba + " " + bb + " < " + stdin
      exit
    end

    if (stdin_b != nil)
      stdin = stdin_b
    end

    txt=bb.split(/[\.]+/)
    if txt.size > 1  #is it SPEC? like 186.crafty
      bb = txt[1]
      if (bb == "gcc")
        bb = "gcc_06"
      end
    end
    if (arg_b != nil)
      options[:arg_b] = "-- " + bb + "  " + arg_b
    else
      options[:arg_b] = "-- " + bb
    end
end

#third core
if (options[:numCores] > 3)

    bc = options[:benchmarks_c][b]
    options[:benchName_c] = bc;


    arg_c = getArg(bc,options[:parsecN])
    stdin_c = getStdin(bc)

    txt=bc.split(/[\.]+/)
    if txt.size > 1  #is it SPEC? like 186.crafty
      bc = txt[1]
      if (bc == "gcc")
        bc = "gcc_06"
      end
    end
    if bc != "dummy"
      if (arg_c != nil)
        options[:arg_c] = "-- " + bc + "  " + arg_c
      else
        options[:arg_c] = "-- " + bc
      end
    else
      options[:arg_c] = " "
    end

#Forth core

    bd = options[:benchmarks_d][b]
    options[:benchName_d] = bd;


    arg_d = getArg(bd,options[:parsecN])
    stdin_d = getStdin(bd)
    if ((stdin != nil and (stdin_c != nil || stdin_d != nil)) or (stdin_c != nil and stdin_d != nil) )
      puts "Error! Only one standard input is allowed!" + " " + ba + " " + bb + " " +  bc + " " + bd + " < " + stdin
      exit
    end

    if (stdin_c != nil)
      stdin = stdin_c
    elsif stdin_d != nil
      stdin = stdin_d
    end

    txt=bd.split(/[\.]+/)
    if txt.size > 1  #is it SPEC? like 186.crafty
      bd = txt[1]
      if (bd == "gcc")
        bd = "gcc_06"
      end
    end
    if bd != "dummy"
      if (arg_d != nil)
        options[:arg_d] = "-- " + bd + "  " + arg_d
      else
        options[:arg_d] = "-- " + bd
      end
    else
      options[:arg_d] = ""
    end
end

#fifth core
if (options[:numCores] > 4)

    be = options[:benchmarks_e][b]
    options[:benchName_e] = be;


    arg_e = getArg(be,options[:parsecN])
    stdin_e = getStdin(be)

    txt=be.split(/[\.]+/)
    if txt.size > 1  #is it SPEC? like 186.crafty
      be = txt[1]
      if (be == "gcc")
        be = "gcc_06"
      end
    end
    if (arg_e != nil)
      options[:arg_e] = "-- " + be + "  " + arg_e
    else
      options[:arg_e] = "-- " + be
    end

#sixth core

    bf = options[:benchmarks_f][b]
    options[:benchName_f] = bf;


    arg_f = getArg(bf,options[:parsecN])
    stdin_f = getStdin(bf)


    txt=bf.split(/[\.]+/)
    if txt.size > 1  #is it SPEC? like 186.crafty
      bf = txt[1]
      if (bf == "gcc")
        bf = "gcc_06"
      end
    end
    if bf != "dummy"
      if (arg_f != nil)
        options[:arg_f] = "-- " + bf + "  " + arg_f
      else
        options[:arg_f] = "-- " + bf
      end
    else
        options[:arg_f] = ""
    end

#seventh core

    bg = options[:benchmarks_g][b]
    options[:benchName_g] = bg;


    arg_g = getArg(bg,options[:parsecN])
    stdin_g = getStdin(bg)


    txt=bg.split(/[\.]+/)
    if txt.size > 1  #is it SPEC? like 186.crafty
      bg = txt[1]
      if (bg == "gcc")
        bg = "gcc_06"
      end
    end
    if bg != "dummy"
      if (arg_g != nil)
        options[:arg_g] = "-- " + bg + "  " + arg_g
      else
        options[:arg_g] = "-- " + bg
      end
    else
        options[:arg_g] = ""
    end

#eighth core

    bh = options[:benchmarks_h][b]
    options[:benchName_h] = bh;


    arg_h = getArg(bh,options[:parsecN])
    stdin_h = getStdin(bh)


    txt=bh.split(/[\.]+/)
    if txt.size > 1  #is it SPEC? like 186.crafty
      bh = txt[1]
      if (bh == "gcc")
        bh = "gcc_06"
      end
    end
    if bh != "dummy"
      if (arg_h != nil)
        options[:arg_h] = "-- " + bh + "  " + arg_h
      else
        options[:arg_h] = "-- " + bh
      end
    else
        options[:arg_h] = ""
    end

    if ((stdin != nil and (stdin_e != nil or stdin_f != nil or stdin_g != nil or stdin_h != nil )) or (stdin_e != nil and stdin_f != nil and stdin_g != nil and stdin_h != nil) )
      puts "Error! Only one standard input is allowed!" + " " + ba + " " + bb + " " +  bc + " " + bd + " " + be + " " + bf + " " +  bg + " " + bh + " < " + stdin
      exit
    end

    if (stdin_e != nil)
      stdin = stdin_c
    elsif stdin_f != nil
      stdin = stdin_f
    elsif stdin_g != nil
      stdin = stdin_g
    elsif stdin_h != nil
      stdin = stdin_h
    end
end

  if (stdin != nil)
    options[:stdin] = "-- stdin " + stdin
  else
    options[:stdin] = ""
  end

  writeScript(options)

end

  runScript = File.new("run-all.sh","w")
  runScript.puts('#!/bin/sh')
  runScript.chmod(0755)
  options[:benchmarks_a].each_index do |b|
    ba =  options[:benchmarks_a][b].split(/[\.]+/) 
    if ba.size > 1 # is it SPEC
      benchmark = ba[1] 
    else
      benchmark = ba[0] 
    end

    if(options[:numCores] > 1)
      bb =  options[:benchmarks_b][b].split(/[\.]+/) 
      benchmark = benchmark + "_" + bb[1]
    end
    if(options[:numCores] > 3)
      bc =  options[:benchmarks_c][b].split(/[\.]+/) 
      if bc.size > 1 # is it SPEC
        benchmark = benchmark + "_" + bc[1]
      else
        benchmark = benchmark + "_" + bc[0]
      end
      bd =  options[:benchmarks_d][b].split(/[\.]+/) 
      if bd.size > 1 # is it SPEC
        benchmark = benchmark + "_" + bd[1]
      else
        benchmark = benchmark + "_" + bd[0]
      end
    end
    if(options[:numCores] > 4)
      be =  options[:benchmarks_e][b].split(/[\.]+/) 
      if be.size > 1 # is it SPEC
        benchmark = benchmark + "_" + be[1]
      else
        benchmark = benchmark + "_" + be[0]
      end
      bf =  options[:benchmarks_f][b].split(/[\.]+/) 
      if bf.size > 1 # is it SPEC
        benchmark = benchmark + "_" + bf[1]
      else
        benchmark = benchmark + "_" + bf[0]
      end
      bg =  options[:benchmarks_g][b].split(/[\.]+/) 
      if bg.size > 1 # is it SPEC
        benchmark = benchmark + "_" + bg[1]
      else
        benchmark = benchmark + "_" + bg[0]
      end
      bh =  options[:benchmarks_h][b].split(/[\.]+/) 
      if bh.size > 1 # is it SPEC
        benchmark = benchmark + "_" + bh[1]
      else
        benchmark = benchmark + "_" + bh[0]
      end
    end

    benchmark = benchmark + "_" + options[:samplerMode] 
    
    benchDir = Dir.getwd + "/" +  benchmark
    
    if options[:local] == true
      command  = "../" + benchmark + "-run.sh > log 2>&1 &" + "\n"
    else
      command  = "qsub -l nodes=1:ppn=2 -l walltime=240:00:00 ../" + benchmark + "-run.sh" + "\n"
    end

    runScript.puts("echo Submitting " + benchmark + "\n" +
                   'if [ ! -d "' + benchDir + '" ]; then' + "\n" +
                   "  mkdir " + benchDir + "\n" +
                   "fi \n" +
                   "cd " + benchDir + "\n" +
                   #"qsub -pe orte 4 ../" + benchmark + "-run.sh" + "\n" +
                   command +
                   "sleep 5" + "\n" +
                   "cd .. \n\n")
  end
  runScript.close

