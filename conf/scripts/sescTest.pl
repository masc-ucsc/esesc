#!/usr/bin/env perl

BEGIN { 
  my $tmp = $0;

  $tmp = readlink($tmp) if( -l $tmp );
  $tmp =~ s/sescTest.pl//;
  unshift(@INC, $tmp)
}


use sesc;
use strict;
use Time::localtime;
use Getopt::Long;

foreach my $tmp (@ARGV) {
  my $cf = sesc->new($tmp);

  printf "cycles=%d\n",$cf->getResultField("OSSimnCPUs");
  printf "cycles=%d\n",$cf->getResultField("Proc(0)clockTicks");
  printf "cycles=%d\n",$cf->getResultField("Proc(0)","clockTicks");

  my $cpuType = $cf->getConfigEntry(key=>"cpucore",index=>0);
  my $fetch   = $cf->getConfigEntry(key=>"fetchWidth",section=>$cpuType);

  # pointed by cpucore[0]
  printf "cpuType=%s : fetch=%d\n",$cpuType,$fetch;

  $cpuType = $cf->getConfigEntry(key=>"cpucore",index=>1);
  $fetch   = $cf->getConfigEntry(key=>"fetchWidth",section=>$cpuType);

  # pointed by cpucore[1]
  printf "cpuType=%s : fetch=%d\n",$cpuType,$fetch;

}
