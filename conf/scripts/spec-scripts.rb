#!/usr/bin/env ruby

# This script is used for generating files to run SPEC CPU2000 and CPU2006
# benchmarks with eSESC.  You can set default values for some of the
# parameters by configuring enviornment variables in your .bashrc file
#
# export ESESC_SCRIPT_EMAIL='youremail@soe.ucsc.edu'
# export ESESC_SCRIPT_CONF=$HOME/projs/esesc/conf/tsample.conf
# export ESESC_SCRIPT_SPECDIR=/mada/software/benchmarks/SPEC
# export ESESC_SCRIPT_OOOXML=$HOME/projs/esesc/conf/OOO.xml
# export ESESC_SCRIPT_ESESCEXE=$HOME/build/esesc-release/main/esesc
# export ESESC_SCRIPT_THERMMAIN=$HOME/build/pwth/libsesctherm/thermmain
# export ESESC_SCRIPT_DOPOWER=true

require 'fileutils'
require 'optparse'

THERM_TT = 363.15 # Thermal throttling threshold 90C HP
THERM_TTLP = 348.15 # Thermal throttling threshold 75 LP

### Set default options ####
options = Hash.new
options[:benchmarks] =[]
options[:email] = ENV['ESESC_SCRIPT_EMAIL']
options[:esescConf] = ENV['ESESC_SCRIPT_CONF']
options[:specDir] =  ENV['ESESC_SCRIPT_SPECDIR']
options[:thermFF] = 1
options[:thermTT] = 0 # no throttling at all
options[:maxSimedTime] = 90 # seconds
options[:stackSize] = "unspecified"
options[:OOOxmlLoc] = ENV['ESESC_SCRIPT_OOOXML']
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

if(options[:email].nil? && options[:esescConf].nil? && options[:specDir].nil? && options[:OOOxmlLoc].nil? && options[:esescExe].nil? && options[:enablePower].nil?)
    puts "NOTE: you can set default values for some parameters using environment variables."
end

##############################
## Parse command line options
##############################
optparse = OptionParser.new do |opts|
  opts.banner = "Usage: cpu2006-scripts.rb [options]"

  opts.on("-b bench1,bench2","--benchmarks bench1,bench2",Array,"specify list of benchmarks to run") do |bench|
    options[:benchmarks] = bench
  end

  opts.on("-e email","--email email","email address to send result to") do |email|
    options[:email] = email
  end

  opts.on("-m mode","--mode mode","sampling mode to use") do |mode|
    options[:samplerMode] = mode
  end

  opts.on("-c conf_file","--conf conf_file","specify location of conf file to use") do |conf|
    options[:esescConf] = conf
  end

  opts.on("-d specDir","--specDir specDir","specify location of SPEC CPU benchmarks") do |dir|
    options[:specDir] = dir
  end

  opts.on("-x esesc-exe","--exe esesc_exe","specify location of esesc executable") do |exe|
    options[:esescExe] = exe
  end

  opts.on("-v jobname","--extra","extra variable to append to the name of the job") do |jobname|
    options[:jobname] = "-" + jobname
  end

  opts.on("-f thermff","--thermff","fast fowarding for thermal sampling") do |thermFF|
    options[:thermFF] = thermFF
  end

  opts.on("-t thermtt","--thermtt","set to 1 to enable thermal throttling for HP-CPU, 2 for LP-CPU") do |thermTT|
    options[:thermTT] = thermTT
  end

  opts.on("-a maxSimedTime","--maxSimedTime","max Simulated time") do |maxSimedTime|
    options[:maxSimedTime] = maxSimedTime
  end

  opts.on("-l OOOxml","--OOOxml","location of OOO.xml file") do |OOOxml|
    options[:OOOxmlLoc] = OOOxml
  end

  opts.on("-p enablePower","--enablePower","setting for whether power/thermal should be run") do |enablePower|
    options[:enablePower] = enablePower
  end

  opts.on("-z labelDir","--labelFile","specify location of label files") do |labelDir|
    options[:labelDir] = labelDir
  end

  opts.on("-i thermInputDir","--thermDir","specify directory root for thermal input") do |thermDir|
    options[:thermDir] = thermDir
  end

  opts.on("-k thermmain","--thermmain","location of thermmain") do |thermmain|
    options[:thermmain] = thermmain
  end

  opts.on("-w powerSuffix","--powerSuffix","suffix for power files read by sesctherm") do |powerSuffix|
    options[:powerSuffix] = powerSuffix
  end

  opts.on("-s stackSize","--stack","set stack size used by QEMU") do |stackSize|
    options[:stackSize] = stackSize
  end

  opts.on("-q qemuExe","--qemu-exe","location of qemu executable for emulation only") do |qemuExe|
    options[:qemuExe] = qemuExe
  end

  opts.on("-g postfix","--post-fix","postfix to benchmark binaries") do |postfix|
    options[:postfix] = postfix
  end

  opts.on("-j inputPath","--inputPath","path to input files") do |inputPath|
    options[:inputPath] = inputPath
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
### Function for Coskun scripts
######################################
def coskunScript(options,benchName,baseName,spointSection)

  scriptFile = File.new((benchName + "-coskun.sh"),"w")
  benchDir = Dir.getwd + "/" + benchName

  ## Commands for qsub
  scriptFile.puts('#!/bin/sh ' + "\n" +
                  '#$ -S /bin/sh' + "\n" +
                  '#$ -m es' + "\n" +
                  '#$ -M ' + options[:email] + "\n" +
                  '#$ -cwd' + "\n" +
                  '#$ -o ' + baseName + ".log \n" +
                  '#$ -e ' + baseName + ".err \n" +
                  '#$ -N ' + baseName + options[:jobname] + "\n\n")


  scriptFile.puts('if [ ! -d "' + benchDir + '" ]; then' + "\n" +
                  "  mkdir " + benchDir + "\n" +
                  "fi \n")
  scriptFile.puts("cd " + benchDir)

  scriptFile.puts('export ESESC_ReportFile="' + baseName + options[:jobname] + '"')


  #labelFile = options[:labelDir] + "/" + benchName + "/" + benchName + "-100m.labels"
  labelFile = Dir.glob(options[:labelDir] + benchName + "/" + "*_labels")
  if(File.exist?(labelFile.to_s))
    labelFile = File.expand_path(labelFile.to_s)
  else
    puts "ERROR: '#{labelFile} not found"
    exit(1)
  end

  if(options[:powerSuffix].nil?)
    options[:powerSuffix] = ""
  end

  if(options[:thermDir].nil?)
    puts "ERROR: did not specify location of power files"
    exit(1)
  end
  thermDir = options[:thermDir] + "/" + benchName
  if(File.exist?(thermDir))
    thermDir = File.expand_path(thermDir)
  end

  leakageFile = Dir.glob(thermDir + "/power_lkg_esesc_" + baseName + options[:powerSuffix] + ".*")
  if(leakageFile.length != 1)
    puts "ERROR: problem with leakage file: " + leakageFile
    exit(1)
  end

  deviceFile = Dir.glob(thermDir + "/deviceTypes_esesc_" + baseName + options[:powerSuffix] + ".*")
  if(deviceFile.length != 1)
    puts "ERROR: problem with device file: " + deviceFile
    exit(1)
  end

  scriptFile.puts(options[:thermmain] +
                  " -c " + options[:esescConf] +
                  " -s " + labelFile +
                  " -t " + "esesc_" + baseName + options[:powerSuffix] +
                  " -p " + thermDir +
                  " -l " + leakageFile[0] +
                  " -d " + deviceFile[0] +
                  " -m " + spointSection)

  scriptFile.puts("exit 0")
  scriptFile.chmod(0755)
  scriptFile.close

end

######################################
### Function to create script file ###
######################################
def writeScript(options,benchName,version,exeArgs,input)

  re1='(\\d+)'
  re2='.*?'
  re3='((?:[a-z][a-z0-9_]*))'
  re=(re1+re2+re3)
  benchRegex = Regexp.new(re,Regexp::IGNORECASE)
  benchMatch = benchRegex.match(benchName)
  benchSpoint = benchMatch[1].to_s + benchMatch[2].to_s
  postFix = options[:postfix]
  if(options[:esescConf]=="")
    esesc_conf_path = ""
  else
    esesc_conf_path = " -c " + options[:esescConf]
  end

  if(postFix==nil)
    benchExe = benchMatch[2].to_s + "_base.sparc_linux"
  else
    benchExe = benchMatch[2].to_s + postFix
  end
  benchSuffix = benchMatch[2].to_s


  specDir = options[:specDir]
  if(specDir==nil)
    specDir = "/mada/software/benchmarks/SPEC/cpu2000-sparc/exe/" #default option
  end
  if(options[:samplerMode].casecmp("Spoint")== 0)
		benchSampler = "SPointMode" + benchSpoint
	elsif(options[:samplerMode].casecmp("Coskun") == 0)
		benchSampler = "SPointMode" + benchSpoint
    coskunScript(options,benchName,benchSuffix,benchSampler)
    return
  else
    benchSampler = options[:samplerMode]
  end

  stackSize = ""
  if(options[:stackSize].to_s != "unspecified" )
    stackSize = "-s " + options[:stackSize].to_s + " "
  end

  if(exeArgs != "")
    exeArgs = " " + exeArgs
  end


  ### Script to run eSESC ####
  benchDir = Dir.getwd + "/" + benchName
  if(benchSampler != 'qemu')
   scriptFile = File.new((benchName + "-run.sh"),"w")

    ## Commands for qsub
    scriptFile.puts('#!/bin/sh ' + "\n" +
                    '#$ -S /bin/sh' + "\n" +
                    '#$ -m es' + "\n" +
                    '#$ -M ' + options[:email] + "\n" +
                    '#$ -cwd' + "\n" +
                    '#$ -o ' + benchSuffix + ".log \n" +
                    '#$ -e ' + benchSuffix + ".err \n" +
                    '#$ -N ' + benchSuffix + options[:jobname] + "\n\n")

    scriptFile.puts('if [ ! -d "' + benchDir + '" ]; then' + "\n" +
                    "  mkdir " + benchDir + "\n" +
                    "fi \n")
    scriptFile.puts("cd " + benchDir)

    scriptFile.puts("cp -f " + options[:OOOxmlLoc] + " .")

    inputpath = options[:inputPath]
    if(inputpath==nil)
      inputpath = specDir + "input/"
    end
    scriptFile.puts("cp -rf " + inputpath + benchName + '/* .')
    scriptFile.puts("cp -f " + specDir + "exe/" + benchExe + " .")
    scriptFile.puts("export ESESC_ReportFile=\"" + benchSuffix + options[:jobname] + "\"")

    if(stackSize.nil?)
      scriptFile.puts("export ESESC_BenchName=\"" + benchExe + exeArgs + "\"")
    else
      scriptFile.puts("export ESESC_BenchName=\"" + stackSize + benchExe + exeArgs + "\"")
    end
    puts("esescExe is" + options[:esescExe] + "\n")
    scriptFile.puts(options[:esescExe] + esesc_conf_path + " " + input)
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
    qemuRun.puts("cp -rf " + specDir + "input/" + benchName + '/* .')
    qemuRun.puts("cp -f " + specDir + "exe/" + benchExe + " .")
    qemuRun.puts(options[:qemuExe] + " " + benchExe + exeArgs + " " + input)
    qemuRun.puts("exit 0")
    qemuRun.chmod(0755)
    qemuRun.close
  end
end
############################################

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

if(options[:OOOxmlLoc]==nil)
  options[:OOOxmlLoc] = ""
end

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

#### CPU2000 benchmarks ####
cintBenchmarks00 = Array.[]("164.gzip","175.vpr","176.gcc","179.art","181.mcf","186.crafty","197.parser",
                         "252.eon","256.bzip2","300.twolf")
cfpBenchmarks00 = Array.[]("168.wupwise","171.swim","172.mgrid","173.applu","177.mesa","178.galgel","183.equake",
                        "187.facerec","188.ammp","189.lucas","191.fma3d","200.sixtrack","301.apsi")

tsampleSet00 = Array.[]("176.gcc","171.swim","177.mesa","187.facerec","189.lucas","300.twolf",
                      "186.crafty","164.gzip","175.vpr","172.mgrid","173.applu","256.bzip2") # Both Varied and Smooth
tsampleVSet00 = Array.[]("176.gcc","171.swim","177.mesa","187.facerec","189.lucas","173.applu","256.bzip2","172.mgrid") # Varied
tsampleSSet00 = Array.[]("300.twolf","186.crafty","164.gzip","175.vpr") # Smooth

armall00 = Array.[]("164.gzip","176.gcc","179.art","181.mcf","186.crafty","197.parser","256.bzip2","300.twolf","168.wupwise","171.swim",
                  "172.mgrid","173.applu","177.mesa","183.equake","254.gap","253.perlbmk","189.lucas")


##### CPU  benchmarks ####
cintBenchmarks06 = Array.[]("400.perlbench","401.bzip2","403.gcc","429.mcf",
                           "456.hmmer","458.sjeng","462.libquantum",
                           "464.h264ref","471.omnetpp","473.astar")


cfpBenchmarks06 = Array.[]("433.milc","435.gromacs","444.namd",
                         "447.dealII","450.soplex","453.povray","470.lbm")

armall06 = Array.[]("473.astar","401.bzip2","445.gobmk","456.hmmer","437.leslie3d","429.mcf","444.namd","453.povray","450.soplex","482.sphinx3",
                    "410.bwaves","403.gcc","464.h264ref","470.lbm","462.libquantum","433.milc","471.omnetpp","458.sjeng","998.specrand","999.specrand")

#tsampleSet06 = Array.[]("403.gcc", "458.sjeng", "433.milc", "429.mcf","447.dealII", #varied
tsampleSet06 = Array.[]("403.gcc", "433.milc", "429.mcf","447.dealII", #varied
										 	"400.perlbench", "450.soplex" , "473.astar", "453.povray",  #smooth
										  "444.namd" , "464.h264ref")
tsampleVSet06 = Array.[]("403.gcc", "458.sjeng", "433.milc", "429.mcf","447.dealII") #varied
tsampleSSet06 = Array.[]("400.perlbench", "450.soplex" , "473.astar", "453.povray",  #smooth
										  "444.namd" , "464.h264ref")

all = cintBenchmarks00 + cfpBenchmarks00 + cintBenchmarks06 + cfpBenchmarks06

#### CPU benchmarks for mixed workloads (with GPU apps) #####

gpumix = Array.[]("173.applu", "179.art", "401.bzip2","186.crafty","183.equake",
                  "254.gap","176.gcc","164.gzip","189.lucas","181.mcf","177.mesa",
                  "172.mgrid", "197.parser","253.perlbmk","171.swim", "300.twolf",
                  "168.wupwise")

## Set benchmarks to run
options[:benchmarks] = case options[:benchmarks][0]
  when "all-cpu2006"       then tsampleSet06
  when "tsample-cpu2006v"  then tsampleVSet06
  when "tsample-cpu2006s"  then tsampleSSet06
  when "all-cpu2000"       then tsampleSet00
  when "arm-cpu2000-all"   then armall00
  when "arm-cpu2006-all"   then armall06
  when "tsample-cpu2000v"  then tsampleVSet00
  when "tsample-cpu2000s"  then tsampleSSet00
  when "tsample"           then tsampleSet00 + tsampleSet06
#  when "gpumix"            then gpumix
  when "all"               then all
  else options[:benchmarks] = options[:benchmarks]
end

## Create scripts
options[:benchmarks].each do |benchmark|
  puts "Creating script for: " + benchmark
  case benchmark
    ### SPEC CINT 2000 that are working
    when "164.gzip"
      writeScript(options,benchmark,2000,"input.program 60","")
    when "175.vpr"
      writeScript(options,benchmark,2000,"net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2","")
    when "176.gcc"
      writeScript(options,benchmark,2000,"166.i -o 166.s","")
    when "181.mcf"
      writeScript(options,benchmark,2000,"inp.in","")
    when "186.crafty"
      writeScript(options,benchmark,2000,"","< crafty.in")
    when "197.parser"
      writeScript(options,benchmark,2000,"2.1.dict -batch"," < ref.in")
    when "252.eon"
      writeScript(options,benchmark,2000,"chair.control.cook chair.camera chair.surfaces chair.cook.ppm ppm pixels_out.cook","")
    when "253.perlbmk"
      writeScript(options,benchmark,2000,"-I./lib diffmail.pl 2 550 15 24 23 100","")
    when "254.gap"
      writeScript(options,benchmark,2000,"-l ./ -q -m 192M"," < ref.in")
    when "256.bzip2"
      writeScript(options,benchmark,2000,"input.source 58","")
    when "300.twolf"
      writeScript(options,benchmark,2000,"ref","")

    ### SPEC CFP 2000 that are working
    when "168.wupwise"
      writeScript(options,benchmark,2000,"","")
    when "171.swim"
      writeScript(options,benchmark,2000,"","< swim.in")
    when "172.mgrid"
      writeScript(options,benchmark,2000,"","< mgrid.in")
    when "173.applu"
      writeScript(options,benchmark,2000,"","< applu.in")
    when "177.mesa"
      writeScript(options,benchmark,2000,"-frames 1000 -meshfile mesa.in -ppmfile mesa.ppm","")
    when "178.galgel"
      writeScript(options,benchmark,2000,"","< galgel.in")
    when "179.art"
      writeScript(options,benchmark,2000,"-scanfile c756hel.in -trainfile1 a10.img -trainfile2 hc.img -stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10","")
    when "183.equake"
      writeScript(options,benchmark,2000,"","< inp.in")
    when "187.facerec"
      writeScript(options,benchmark,2000,"","< ref.in")
    when "188.ammp"
      writeScript(options,benchmark,2000,"","< ammp.in")
    when "189.lucas"
      writeScript(options,benchmark,2000,"","< lucas2.in")
    when "191.fma3d"
      writeScript(options,benchmark,2000,"","")
    when "200.sixtrack"
      writeScript(options,benchmark,2000,"","< inp.in")
    when "301.apsi"
      writeScript(options,benchmark,2000,"","")

  ### SPEC CINT 2006 that are working
    when "400.perlbench"
      writeScript(options,benchmark,2006,"-I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1","")
    when "401.bzip2"
      writeScript(options,benchmark,2006,"input.source 280","")
    when "403.gcc"
      writeScript(options,benchmark,2006,"166.i -o 166.s","")
    when "429.mcf"
      writeScript(options,benchmark,2006,"inp.in","")
    when "445.gobmk"
      writeScript(options,benchmark,2006,"--quiet --mode gtp 13x13.tst","")
    when "456.hmmer"
      writeScript(options,benchmark,2006,"nph3.hmm swiss41","")
    when "458.sjeng"
      writeScript(options,benchmark,2006,"ref.txt","")
    when "462.libquantum"
      writeScript(options,benchmark,2006,"1397 8","")
    when "464.h264ref"
      writeScript(options,benchmark,2006,"-d foreman_ref_encoder_baseline.cfg","")
    when "471.omnetpp"
      writeScript(options,benchmark,2006,"omnetpp.ini","")
    when "473.astar"
      writeScript(options,benchmark,2006,"BigLakes2048.cfg","")

    ### SPEC CFP 2006 that are working
    when "410.bwaves"
      writeScript(options,benchmark,2006,"","")
    when "433.milc"
      writeScript(options,benchmark,2006,""," < su3imp.in")
    when "434.zeusmp"
      writeScript(options,benchmark,2006,"","")
    when "435.gromacs"
      writeScript(options,benchmark,2006,"-silent -deffnm gromacs -nice 0","")
    when "444.namd"
      writeScript(options,benchmark,2006,"--input namd.input --iterations 38 --output namd.out","")
    when "447.dealII"
      writeScript(options,benchmark,2006,"23","")
    when "450.soplex"
      writeScript(options,benchmark,2006,"-s1 -e -m45000 pds-50.mps","")
    when "453.povray"
      writeScript(options,benchmark,2006,"SPEC-benchmark-ref.ini","")
    when "470.lbm"
      writeScript(options,benchmark,2006,"3000 reference.dat 0 0 100_100_130_ldc.of","")
    when "437.leslie3d"
      writeScript(options,benchmark,2006,"","< leslie3d.in")

    else
      puts "WARNING: '#{benchmark}' is not a valid benchmark"
      options[:benchmarks].delete(benchmark)
    end
end
if(options[:samplerMode] == 'qemu')
  qemuRunScript = File.new("qemu-run-all.sh","w")
  qemuRunScript.puts('#!/bin/sh')
  qemuRunScript.chmod(0755)
  options[:benchmarks].each do |benchmark|
    benchDir = Dir.getwd + "/" + benchmark
    qemuRunScript.puts("echo Submitting " + benchmark + "\n" +
                       'if [ ! -d "' + benchDir + '" ]; then' + "\n" +
                       "  mkdir " + benchDir + "\n" +
                       "fi \n" +
                       "cd " + benchDir + "\n" +
                       "qsub ../" + benchmark + "-qemurun.sh" + "\n" +
                       "sleep 1" + "\n" +
                       "cd .. \n\n")
  end
  qemuRunScript.close
else if(options[:samplerMode] == 'Coskun')
       runScript = File.new("run-all.sh","w")
       runScript.puts('#!/bin/sh')
       runScript.chmod(0755)
       options[:benchmarks].each do |benchmark|
         benchDir = Dir.getwd + "/" + benchmark
         runScript.puts("echo Submitting " + benchmark + "\n" +
                        'if [ ! -d "' + benchDir + '" ]; then' + "\n" +
                        "  mkdir " + benchDir + "\n" +
                        "fi \n" +
                        "cd " + benchDir + "\n" +
                        "qsub -pe orte 2 ../" + benchmark + "-coskun.sh" + "\n" +
                        "sleep 1" + "\n" +
                        "cd .. \n\n")
       end
  runScript.close
else #if(options[:samplerMode] != 'qemu')
  runScript = File.new("run-all.sh","w")
  runScript.puts('#!/bin/sh')
  runScript.chmod(0755)
  options[:benchmarks].each do |benchmark|
    benchDir = Dir.getwd + "/" + benchmark
    runScript.puts("echo Submitting " + benchmark + "\n" +
                   'if [ ! -d "' + benchDir + '" ]; then' + "\n" +
                   "  mkdir " + benchDir + "\n" +
                   "fi \n" +
                   "cd " + benchDir + "\n" +
                   "qsub -pe orte 2 ../" + benchmark + "-run.sh" + "\n" +
                   "sleep 1" + "\n" +
                   "cd .. \n\n")
  end
  runScript.close
end
end

