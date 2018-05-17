#!/usr/bin/ruby -i
#
# A wrapper for the floorplanner
# By Ehsan K. Ardestani

if ARGV.size != 3
  puts "Usage #{$0} BuildDirPath SrcDirPath nameMangle >> your/flp.conf"
  exit
end

BuildDir = ARGV[0]
SrcDir = ARGV[1]
FLP = "sysFlp.desc"
TDP = "sysTDP.p"
NewFLP = "new.flp"
MSG=ARGV[2]

FlpConv = File.join(SrcDir, "conf", "scripts", "flpconv.rb")
HotFloorplan = File.join(BuildDir,"floorplan", "hotfloorplan")
Config = File.join(BuildDir,"floorplan", "hotspot.config")



# check if hotfloorplan exist
# if not, make hotfloorplan
if !File.exist?(HotFloorplan)
  puts "Looking for the floorplanner #{HotFloorplan}"
  puts "the floorplanner is not compiled"
  puts "make floorplan"
  exit
end

# run hotfloorplan
system("#{HotFloorplan} -f #{FLP} -p #{TDP} -o #{NewFLP} -c #{Config} > log")

# run flpconv > flp.conf
convcmd = "#{FlpConv} #{NewFLP} #{MSG}"
res = `#{convcmd}`
puts res
