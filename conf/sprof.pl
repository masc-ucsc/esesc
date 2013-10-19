#!/usr/bin/env perl

BEGIN {
  my $tmp = $0;

  $tmp = readlink($tmp) if( -l $tmp );
  $tmp =~ s/sprof.pl//;
  unshift(@INC, $tmp)
}

use sesc;
use strict;
use Time::localtime;
use Getopt::Long;

# TODO:
#
# Interesting data to report:
#
# Avg number of times that an instruction is executed
# % of instructions in the critical path
#
# Show the top X% of the most executed instr
# Show the top X% of branches that cause more penalty
# Show the top X% of loads with higher memory time
# Show the top X% of loads with higher resource time
#
# Add all the values at functions, and do the same rankings.
#

use strict;
use Getopt::Long;

my $tmp;

my @addr2func;
my %funcStats;
my %funcCalls;
my %funcTime;
my @addrStats;

# See BinStats::save format
my $dataLen;
my $dataFields;
my $resultFile;
my $startAddr=0x10000160; # Typical case, but not always

my $op_dump;
my $op_help;
my $op_issue;
my $op_bp;
my $op_sort="CLK";
my $op_time=1;  # 1% by default
my $op_factor;
my $op_xtra;
my $op_comp;

my $result = GetOptions("dump",\$op_dump,
			"issue=i",\$op_issue,
			"bp=i",\$op_bp,
			"sort=s",\$op_sort,
			"time=f",\$op_time,
			"factor=f",\$op_factor,
			"comp",\$op_comp,
			"xtra",\$op_xtra,
			"help",\$op_help
                       );



my $dataFile;

main();
exit(0);

sub processParams {
  my $badparams=0;

  if( @ARGV == 0 ) {
    print "Specify the data to process\n";
    $badparams =1;
  }else{
    $dataFile = $ARGV[0];

    unless( -e $dataFile ) {
      print "Impossible to find the data file [$dataFile]\n";
      $badparams =1;
    }
  }

  if( defined $dataFile ) {
    open(STATFILE,"<" . $dataFile) or die("I can't open data file [${dataFile}]");

    my $resultFile = <STATFILE>;
    chop($resultFile);
	
    my $data;
    read(STATFILE, $data , 4);
    my @field = unpack("N",$data);
    $dataLen = $field[0];

    if( $dataLen < 4+4+4+4+4 ) {
      print "Invalid number of arguments check BinStats\n";
      $badparams=1;
    }

    close(STATFILE);
  }

  for(my $i=0;$i<$dataLen;$i+=4) {
    $dataFields .= "N";
  }

  if( $op_sort ) {
    $op_sort = uc $op_sort;
  }

  if( $op_time < 0 || $op_time > 100 ) {
    print "Invalid time margin. It must be between 0 and 100\n";
    $badparams =1;
  }

  $op_factor = 1 unless( $op_comp );
  if( defined $op_factor and $op_comp ) {
    print "You can not compensate automatically and specify a factor simulataneously\n";
    $badparams =1;
  }

  if( $op_help or $badparams ) {
    print "./sprof.pl [options] <binData> [binary]\n";
    print "\t-dump                : Dump all the instructions statistics\n";
    print "\t-issue=<int>         : Processor issue\n";
    print "\t-sort=[ipc|ctrl]     : Alternative sorting methods\n";
    print "\t-time=<f>            : Minimum Time(%) to show\n";
    print "\t-xtra                : Show extra data about the app\n";
    print "\t-comp                : Compensate inacuracy of measurement\n";
    print "\t-help                : Show this help\n";
    exit 0;
  }
}

sub instID2Addr {
  my $id = shift;

  return $startAddr+($id<<2);
}

sub calcIPC {
  my $func = shift;

  my $nInst = $funcStats{$func}[0];

  my $ipc = $nInst/calcCLK($func);

  return $ipc;
}

sub calcCLK {
  my $func = shift;

  my $nInst = $funcStats{$func}[0];

  my $clk = $funcTime{$func};

#   if( $nInst > $op_issue*$clk or $clk == 0 ) {
#     my $brMiss= $funcStats{$func}[2];

#     my $guessIPC = $op_issue*$nInst/($brMiss+$nInst);

#     $clk = $funcStats{$func}[0]/$guessIPC;
#   }
  $clk *= $op_factor;

  return $clk;
}

sub calcCTRL {
  my $func = shift;

  my $brMiss= $funcStats{$func}[2];

  my $ctrl = $brMiss/($op_issue*calcCLK($func));

  return $ctrl;
}

my $totalTime;

sub main {

  processParams();

  open(STATFILE,"<" . $dataFile) or die("I can't open data file [${dataFile}]");

  getHeader();

  readStats();

  close(STATFILE);

  my $tmp;
  my @field;

  unless( $op_factor ) {
    $op_factor = 1;
    my $total=0;
    foreach $tmp (keys %funcStats) {
      $total += calcCLK($tmp);
    }
    $op_factor = sesc::getResultField($resultFile,"OSSim","nCycles")/ $total;
  }

  if( $op_sort =~ /IPC/ ) {
    @field = sort { calcIPC($b) <=> calcIPC($a) } keys %funcStats;
  }elsif( $op_sort =~ /CTRL/ ) {
    @field = sort { calcCTRL($a) <=> calcCTRL($b) } keys %funcStats;
  }else{
    @field = sort { calcCLK($b) <=> calcCLK($a) } keys %funcStats;
  }

  $totalTime=0;

  print "Associated result file: ${resultFile}\n";
  print "              Function    nCalls   call(clk)   IPC Acum(%) Time(%) Busy(%) Ctrl(%) Othr(%)";
  if( $op_xtra ) {
    print "  IF-SC  SC-EX\n";
  }else{
    print "\n";
  }
  foreach $tmp (@field) {
    dumpFunction($tmp);
  }
}

sub dumpFunction {
  my $func = shift;

  return unless( defined $funcStats{$func} );
  return unless( $funcCalls{$func} > 0 );

  my $absTime = calcCLK($func);
  my $time = 100*$absTime/sesc::getResultField($resultFile,"OSSim","nCycles");

  return if( $time < $op_time );

  my $nInst = $funcStats{$func}[0];
  my $exe   = $funcStats{$func}[1];
  my $brMiss= $funcStats{$func}[2];

  printf "%22s ", $func;

  printf "%9.0f ", $funcCalls{$func};

  printf " %10.1f ", $absTime/$funcCalls{$func};

  my $ipc = calcIPC($func);
  printf " %3.2f ", $ipc;

  $totalTime+=$time;
  printf "%6.2f%% ", $totalTime;
  printf "%6.2f%% ", $time;

  my $tmp;
  my $total=100;

  # Busy breakdown
  $tmp = 100*$ipc/$op_issue;
  $total -= $tmp;
  printf " %5.2f%% ", $tmp;

  # Control breakdown
  $tmp = 100*$brMiss/($absTime*$op_issue)/$op_factor;
  $total -= $tmp;
  printf " %5.2f%% ", $tmp;

  # Other breakdown
  printf " %5.2f%% ", $total;


  if( $op_xtra ) {
    # Avg Exe Time
    printf "%6.2f", ($exe/$nInst);
  }
  print "\n";
}

sub readStats {
  my $data;

  if( $op_dump ) {
    print "Address : instruction address (not all of them only executed)\n";
    print "times   : # times that the instruction has been executed\n";
    print "SC-EX   : Avg time the instruction from Window to executed (!= retired)\n";
    print "branch  : Avg instructions not fetched because branch penalty\n";
    print "Address : times:  SC-EX: branch\n";
  }

  unless( defined $op_issue ) {
    my $issue = sesc::configEntry($resultFile,"fetchWidth");
    my $temp = sesc::configEntry($resultFile,"issueWidth");
    if( $temp < $issue && $temp ) {
      $issue = $temp;
    }
    $op_issue = $issue;
  }

  unless( defined $op_bp ) {
    $op_bp = sesc::configEntry($resultFile,"branchDelay");
  }
  $op_bp--;

  my $cFunc = "__start";
  while(1){
    my $len = read(STATFILE, $data, $dataLen);

    unless( $len ) {
      last;
    }

    my @field = unpack($dataFields,$data);

    my $addr = $field[0];
    my $inst = $field[1];


    $addrStats[$addr] = @field[2..5];
    if( defined $addr2func[$addr] ) {
      $cFunc = $addr2func[$addr];
      if( $op_dump ) {
	print "Function [$cFunc] cycles=" . $funcTime{$cFunc};
	print " calls=" . $funcCalls{$cFunc};
	print "\n";
      }
    }

    if( $op_dump ) {
      if( $field[2] == 0 ) {
	printf "Invalid nExec for %x\n",instID2Addr($field[0]);
	die;
      }
      printf("%x:%6ld:%7.2f:%7.2f",instID2Addr($field[0])
	     ,$field[2]
	     ,($field[3]/$field[2])
	     ,(100*$field[4]/$field[2])
	    );
      for(my $i=6*4;$i<$dataLen;$i+=4 ){
	printf(":%d",$field[$i/4]);
      }
      printf("  \t %s\n",sesc::mipsDis(instID2Addr($field[0]),$field[1]));
    }

    if( defined $funcStats{$cFunc} ) {
      $funcStats{$cFunc}[0] += $field[2];
      $funcStats{$cFunc}[1] += $field[3];
      $funcStats{$cFunc}[2] += $field[4];
    }else{
      $funcStats{$cFunc}[0] = $field[2];
      $funcStats{$cFunc}[1] = $field[3];
      $funcStats{$cFunc}[2] = $field[4];
    }
  }
}

sub getHeader {
  my $tmp;
  my $startAddr;

  my $data;
  my $addr;

  $resultFile = <STATFILE>;
  chop($resultFile);

  # ignore dataLen field
  read(STATFILE, $data, 4) == 4 or die("Invalid header");

  read(STATFILE, $data , 16) == 16 or die("Invalid header function packet");
  my @field = unpack("NNNN",$data);

  $startAddr = $field[0];

  my $name = <STATFILE>;
  chop($name);

  if( $field[2] > 0  or $field[3] > 0 ) {
    $addr = ($field[0]-$startAddr)>>2;
    $addr2func[$addr] = $name;
    $funcCalls{$name} = $field[1];
    $funcTime{$name}  = $field[2]*(1024*1024*1024)+$field[3];
  }

  while(1){
    read(STATFILE, $data , 16) == 16 or die("Invalid header function packet");
    @field = unpack("NNNN",$data);
    last if( $field[0] == 0 );

    $name = <STATFILE>;
    chop($name);

    if( $field[2] > 0  or $field[3] > 0 ) {
      $addr = ($field[0]-$startAddr)>>2;
      $addr2func[$addr] = $name;
      $funcCalls{$name} = $field[1];
      $funcTime{$name}  = $field[2]*(1024*1024*1024)+$field[3];
    }
  }
}
