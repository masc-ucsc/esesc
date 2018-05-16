#!/usr/bin/ruby -i


# This is to convert a floorplan output of hotspot or
# QUILT to esesc readable format.
# example:
# flpconv foo.flp _foo >> flp.conf
#
# the output is like:
#
# [floorplan_foo]
# ..
# ..
#
# if no modifier message is provided (_foo)
# a random number will be generate for that.
#
# Author: Ehsan K.Ardestani
#

if (ARGV.size != 1 and ARGV.size !=2)
  puts "Usage: " + $0 + "  foo.flp [string]"
  exit
end

File.open(ARGV[0], "r") { |iflp|

	str = ARGV.size==2?ARGV[1]:rand(100).to_s
	i = 0

	blocks = Array.new

	puts "\n\n[floorplan" + str + "]"


	while(line = iflp.gets)

		line = line.lstrip
		line = line.chomp


		if line[0] == 35 # '#'
			puts line
			next;
		end


		txt = line.split(/[\ \t]+/)
		blocks.push(txt[0]) # for blockMatch
		print "blockDescr[" + i.to_s + "]  = \""
		txt.each{|t|
			print t + " "
		}
		print "\"\n"

		i = i + 1

	end




	puts
	puts
	puts "# densities are scaled based upon average chip transistor density"
	puts "# NOTE: all values HAVE to be normalized to 1 (nothing less than 1)"

	for ii in 0..(i-1)
		puts "blockchipDensity[" + ii.to_s + "] = 1.00"
	end

	puts "# densities are scaled based upon average chip interconnect transistor density"
	for ii in 0..(i-1)
		puts "blockinterconnectDensity[" + ii.to_s + "] = 1.00"
	end
	#
	###################### NOW DO blockMatch
	puts
	puts "[layoutDescr#{str}]"
	i = 0
	blocks.each{ |block|
		puts "blockMatch[#{i}] = \"#{block}\""
		i+=1
	}
	puts
	######################
}


