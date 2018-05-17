#!/usr/bin/ruby
#This script parses the bbv file and generates simpoint output

require "fileutils"
require "optparse"

# Parse command line options
options = {}
optparse = OptionParser.new do |opts|
  opts.banner = "Useage: bbb-editor.rb [options]"

  opts.on("-b bench","--benchmark bench","name of benchmark to generate simpoints for") do |bench|
    options[:bench] = bench
  end

  options[:bbv] = "qemusim.bbv"
  opts.on("-i bbv_file","--bbv bbv_file","name of bbvfile, default = " + options[:bbv]) do |bbv|
    options[:bbv] = bbv
  end

  options[:conf] = "spoints.conf"
  opts.on("-c config_file","--conf config_file","name of config file to output simpoints to, default = " +
          options[:conf]) do |conf|
    options[:conf] = conf
  end

  opts.on('-h','--help','Display this screen') do
    puts opts
    exit
  end

  if ARGV.empty?
    puts opts
    exit
  end
end
optparse.parse!

simpointBin = "/mada/users/gsouther/tools/simpoint/SimPoint.3.2/bin/simpoint"
benchName = options[:bench]

#SescConf doesn't like periods so we need to get rid of them
re1='(\\d+)'
re2='.*?'
re3='((?:[a-z][a-z0-9_]*))'
re=(re1+re2+re3)
benchRegex = Regexp.new(re,Regexp::IGNORECASE)
benchMatch = benchRegex.match(benchName)
benchSpoint = benchMatch[1].to_s + benchMatch[2].to_s

inputFile = File.open(options[:bbv],"r")
outFile   = File.new((benchName + "-100b.bbv"),"w")

# Limit bbv file to 1000 lines which is the same as 100,000,000,000
# instructions for simpoint size of 100,000,000
puts "Parsing bbv file"
line = inputFile.gets
count = 0
while(line != nil && count < 1000)
  outFile.puts(line)
  count = count + 1
  line = inputFile.gets
end
outFile.close()

#Execute simpoint binary
command = simpointBin + " -saveSimpointWeights " + benchName + "-100m.weights" +
  " -saveSimpoints " + benchName + "-100m.simpoints" +
  " -saveLabels " + benchName + "-100m.labels" +
  " -numInitSeeds 1" +
  " -loadFVFile " + benchName + "-100b.bbv" + " -maxK 10"

puts "Running simpoint"
`#{command}`

SPoint = Struct.new(:inst,:weight)

# Reoder simpoints so that they are listed in the order they
# occur during program execution
puts "Writing output"
simpointFile = File.open((benchName+"-100m.simpoints"),"r")
spweightFile = File.open((benchName+"-100m.weights"),"r")
simpointLine = simpointFile.gets
spweightLine = spweightFile.gets
pointsArray = Array.[]
while(simpointLine != nil)
  values = simpointLine.split()
  weight = spweightLine.split()
  spstruct = SPoint.new(values[0].to_i,weight[0].to_f)
  pointsArray.push(spstruct)
  simpointLine = simpointFile.gets
  spweightLine = spweightFile.gets
end
pointsArray = pointsArray.sort(){|x,y| x.inst <=> y.inst}

# Write output to config file
confFile = File.open(options[:conf],"a")

confFile.puts("\n[SPointMode"+benchSpoint+"]")
confFile.puts("type         = SPoint")
confFile.puts("spointSize   = 1e8")
for i in 0 .. pointsArray.length - 1 do
  confFile.puts("spoint[" + i.to_s() +   "]    = " + pointsArray[i].inst.to_s() + "e8")
  confFile.puts("spweight[" + i.to_s() + "]  = " + pointsArray[i].weight.to_s())
end

