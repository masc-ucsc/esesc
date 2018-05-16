#!/usr/bin/ruby -w

require "fileutils"

options = ""
ARGV.each do |ops|
  next unless ops =~ /^-/
  next if ops =~ /^-key=/
  options << " " + ops
end

xtra_key = ""
ARGV.each do |file|
  if file =~ /^-key=/
     xtra_key = "_" + file.split(/=/)[1]
     next
  end
  next if file =~ /^-/

  File.open(file) do |fd|

    f = File.basename(file).split(/_/)
    bench  = f[0]

    spsize = f[1].split(/\./)[0]
    if (spsize == "10M")
      spsize = 10000000
    elsif (spsize == "100M")
      spsize = 100000000
    else
      puts "ERROR: impossible to detect BB size? " + spsize
      exit(-3)
    end

    x = 0
    while line_txt = fd.gets
      line = line_txt.chop().split(/[ ]+/)
      puts "run.pl #{options}" \
      + " -key=X#{x}#{xtra_key}" \
      + " -spoint=" + line[0].to_s \
      + " -spointsize=" + spsize.to_s \
      + " #{bench}"
      x = x + 1
    end
  end

end
