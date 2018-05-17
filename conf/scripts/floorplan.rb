#!/usr/bin/ruby -i
#
#
# Use this script to generate a new floorplan and update the config files
# accordingly.
#
# By Ehsan K. Ardestani

if ARGV.size != 4
  puts "Usage #{$0} BuildDirPath SrcDirPath  RunDirPath nameMangle"
  exit
end


BuildDir = ARGV[0]
SrcDir = ARGV[1]
RunDir = ARGV[2]
MSG=ARGV[3]

esescConf = File.join(RunDir, "esesc.conf")
thermConf = File.join(RunDir, "pwth.conf")
flpConf = File.join(RunDir, "flp.conf")
esesc = File.join(BuildDir, "main", "esesc")
reFloorplan = File.join(SrcDir, "conf", "scripts", "reFloorplan.rb")

cmd = "grep #{MSG} #{flpConf}"
res = `#{cmd}`
if res.size != 0
  puts "Seems that there is a floorplan with the name name mangle (#{MSG})."
  puts "Check the flp.conf, or choose a different mangle"
  exit
end

# generate desc and p files


# Back up esesc.conf
puts "Generating config files..."
cmd = "cp #{esescConf} esesc.bak"
`#{cmd}`

# set  reFloorplan = true and save in tmp1
cmd = "cat #{thermConf} | sed 's/reFloorplan[\ ]*=.*/reFloorplan = true/' > tmp1"
`#{cmd}`

# get a new esesc.conf that points to tmp1
cmd = "cat #{esescConf} | sed 's/pwth.conf/tmp1/' > tmp2"
`#{cmd}`

cmd = "cat tmp2 | sed 's/enablePower.*/enablePower = true/' > tmp"
`#{cmd}`

cmd = "mv tmp esesc.conf"
`#{cmd}`

# run esesc
puts "Generating floorplanning descriptions and power breakdown (might take a few mins)"
cmd = "#{esesc} -c esesc.conf > refloorplan.log 2>&1"
`#{cmd}`

# recover original esesc.conf
puts "reconvering the original config files (except for flp.conf)"
cmd = "mv esesc.bak #{esescConf}"
`#{cmd}`

# run reFloorplan.rb
puts "Running the floorplanner...(Go have a cup of coffee if it is a floorplaning for a big design (e.g. 4-core))"
cmd = "#{reFloorplan} #{BuildDir} #{SrcDir} #{MSG} >> #{flpConf}"
`#{cmd}`

#update pwth.conf
puts "Updating pwth.conf to point to the new floorplan and layoutDescr (...#{MSG})"
cmd = "cat #{thermConf} | sed \"s/^floorplan\\[0\\].*=.*/floorplan[0] = 'floorplan#{MSG}'/\" > tmp"
`#{cmd}`
cmd = "cat tmp| sed \"s/layoutDescr\\[0\\].*=.*/layoutDescr[0] = 'layoutDescr#{MSG}'/\" > tmp3"
`#{cmd}`
cmd = "mv tmp3 pwth.conf"
`#{cmd}`

puts "Done."

