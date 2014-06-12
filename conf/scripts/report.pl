#!/usr/bin/env perl

BEGIN {
  my $tmp = $0;
  my $tmp2;

  $tmp = readlink($tmp) if( -l $tmp );
  $tmp =~ s/report.pl//;
  unshift(@INC, $tmp);

  $tmp2 = $0;
  $tmp2 =~ s/report.pl//;
  $tmp2 .= $tmp;
  unshift(@INC, $tmp2);
}

use sesc;
use strict;
use Time::localtime;
use Getopt::Long;

###########################################
# Parameters section:
# All the parameters start with op_

my $op_all;
my $op_last;
my $op_bpred=1; # Show by default
my $op_cpu=1;   # Show by default
my $op_sim=1;   # Show by default
my $op_inst=1;  # Show by default
my $op_help;
my $op_table;
my $op_chippower;
my $op_chippower_L;
my $op_enPower;
my $op_enTherm;

my $result = GetOptions("a",\$op_all,
    "last",\$op_last,
    "help",\$op_help,
    "table",\$op_table
    );


exit &main();

sub processParams {
  my $badparams=0;

  if( @ARGV == 0 && !( $op_all or $op_last) ) {
    print "Specify the trace to process\n";
    $badparams =1;
  }

  if( $op_last and $op_all ) {
    print "-last and -a are mutually exclusive options\n";
    $badparams =1;
  }

  if( $op_help or $badparams ) {
    usage();
    exit 0;
  }
}

sub usage {
  print "./report.pl [options] <sescDump>\n";
  print "\t-a            : Reports for all the stat files in current directory\n";
  print "\t-last         : Reports the newest stat file in current directory\n";
  print "\t-table        : Statistics table summary (good for scripts)\n";
  print "\t-help         : Show this help\n";
}

###########################################
# Global variables.
my $nCPUs;
my $nSamplers;
my $cpuType;
my $slowdown;
my $nInstTotal;
my $nLoadTotal;
my $nStoreTotal;
my $freq;
my $cf;    # current conf File
my @Samplers = ();
my @SamplerType = ();
my @Processors = ();
my $enable_gpu = 0;
my $gpusampler = 0;

###########################################
# main section:


sub main {

  processParams();

  my @flist;
  if( $op_all ) {
    opendir(DIR,".");
# short by modification date
    @flist = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
    closedir(DIR);
  }elsif( $op_last ) {
    opendir(DIR,".");
# short by modification date
    my @tmp = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
    @flist = ($tmp[@tmp-1]);
    closedir(DIR);
  }else{
    @flist = @ARGV;
  }

  if( @flist == 0 ) {
    print "No stat files to process\n";
    exit 0;
  }

  foreach my $file (@flist) {

    $cf = sesc->new($file);

    my $bench = $cf->getResultField("OSSim","bench");

    unless ($cf->getResultField("OSSim","msecs")) {
      thermStats_table($file) if($op_table);
      next;
    }

    printf "\n********************************************************************************************************\n";
    printf "# File  : %-20s : %20s\n", $file,ctime((stat($file))[9]);

    print "# THESE ARE PARTIAL STATISTICS\n" if( $cf->getResultField("OSSim","reportName") =~ /signal/i );

    $nCPUs= $cf->getResultField("OSSim","nCPUs");
    $nSamplers= $cf->getResultField("OSSim","nSampler");

    @Samplers    = ();
    @SamplerType = ();
    @Processors  = ();
    SeparateSampler();

    my $powersec = $cf->getConfigEntry(key=>"pwrmodel");
    $op_enPower = $cf->getConfigEntry(key=>"doPower",section=>$powersec);
    $op_enTherm = $cf->getConfigEntry(key=>"doTherm",section=>$powersec);

    simStats($file) if( $op_sim );
    branchStats($file) if( $op_bpred );
    instStats($file)  if( $op_inst );
    printf("-----------------------------------------------------------------------------------------------------------------------------------------\n");
    tradCPUStats( $file ) if( $op_cpu );
    
    newtradMemStats($file);

    powerStats($file) if ($op_cpu);

    # The following are not yet checked for correct GPU/multithreaded operation
    thermStats($file) if ($op_cpu);

    # The following are not yet checked for correct GPU/multithreaded operation
    thermStats_table($file) if($op_table);
    showStatReport($file) if( $op_table );
    dumpKIPS($file, "table12") if( $op_table );
    printf "********************************************************************************************************\n\n";

  }
}

sub ptxStats {
  printf(") ");
  my $PTXinst_T = $cf->getResultField("PTXStats","totalPTXinst_T");
  my $PTXinst_R = $cf->getResultField("PTXStats","totalPTXinst_R");
  if (($PTXinst_T+$PTXinst_R) != 0){
    printf("(%5.3f million Rabbit, %5.3f million Timing ,%5.3f million Total PTX Instructions)",($PTXinst_R)/1000000,($PTXinst_T)/1000000,($PTXinst_T+$PTXinst_R)/1000000);
  }
}

sub SeparateSampler{

  for(my $i=0;$i<$nSamplers;$i++) {
    my @proc_array = ();
    my $ptr = \@proc_array;
    push(@Samplers, $ptr);
    push(@SamplerType,0);
  }

  for(my $i=0;$i<$nCPUs;$i++) {
    my @proc_array = ();
    my $ptr = \@proc_array;
    push(@Processors, $ptr);
  }

  for(my $i=0;$i<$nCPUs;$i++) {
    my $loc = $cf->getResultField("OSSim","P(${i})_Sampler");
    my $type = $cf->getResultField("OSSim","P(${i})_Type");
    if ($type == 1){
      $enable_gpu = 1;
      $gpusampler = $loc;
      @SamplerType[$loc] = 1;
    }
    my $ptr = @Samplers[$loc];
    push (@$ptr,$i);

    my $sptr = @Processors[$i];
    push (@$sptr,$loc);

  }

}

sub newtradMemStats {

  my $file = shift;
  printf "********************************************************************************************************\n";

  if ($op_enPower eq "true") {
    printf ("%-15s %-4s %-9s %-9s %9s %-7s,%6s,%6s   %12s %12s\n",
      "Cache",
      "Occ",
      "AvgMemLat",
      "MemAccesses",
      " MissRate",
      " (  RD",
      "WR",
      "    BUS)",
      "Dyn_Pow (mW)",
      "Lkg_Pow (mW)"
    );
  } else {
    printf ("%-15s %-4s %-9s %-9s %9s %-7s,%6s,%6s \n",
      "Cache",
      "Occ",
      "AvgMemLat",
      "MemAccesses",
      " MissRate",
      " (  RD",
      "WR",
      "    BUS)"
    );

  }

  my @mycaches = `grep ":readHit" ${file} | cut -d: -f1 | sort -u | sort -t\\( -k1 -n`;
  my @myIcaches = grep(/I/, @mycaches);
  for my $cache (@myIcaches) {
    newMemStat($cache);
  }
   printf "--------------------------------------------------------------------------------------------------\n";
  my @myDcaches = grep(/D/, @mycaches);
  for my $cache (@myDcaches) {
    newMemStat($cache);
  }
  printf "--------------------------------------------------------------------------------------------------\n";
  my @restcaches = grep(!/I|D/, @mycaches);
  for my $cache (@restcaches) {
    newMemStat($cache);
  }

}

sub newMemStat {
  my $cache = shift;
  $cache =~ s/\r?\n$//;

  if ($cf->getResultField("${cache}","readHit")>0) {
    printf ("%-15s ",$cache);
    printf "%-4.1f ",$cf->getResultField("${cache}_occ","v");

    my $total =
      $cf->getResultField("${cache}","readHit")
      + $cf->getResultField("${cache}","readHalfHit")
      + $cf->getResultField("${cache}","readHalfMiss")
      + $cf->getResultField("${cache}","readMiss")
      + $cf->getResultField("${cache}","writeHit")
      + $cf->getResultField("${cache}","writeHalfHit")
      + $cf->getResultField("${cache}","writeHalfMiss")
      + $cf->getResultField("${cache}","writeMiss")
      + $cf->getResultField("${cache}","busReadMiss")
      + $cf->getResultField("${cache}","busReadHalfMiss")
      + $cf->getResultField("${cache}","busReadHit")+1;

    my $miss = $cf->getResultField("${cache}","readMiss")
      + $cf->getResultField("${cache}","writeMiss")
      + $cf->getResultField("${cache}","busReadMiss");

    my $tmp  = $cf->getResultField("${cache}_avgMemLat","v");
    my $tmp1  = $cf->getResultField("${cache}_avgMemLat","n");
    printf "%-9.1f %-9d %8.2f%    ", $tmp,$tmp1,100*($miss)/$total;

    my $tmp  = $cf->getResultField("${cache}","readHit");
    my $tmp2 = $cf->getResultField("${cache}","readMiss") 
      + $cf->getResultField("${cache}","readHit")+1;
    printf "(%5.1f%,"  , (100*$tmp/$tmp2);

    my $tmp = $cf->getResultField("${cache}","writeHit");
    my $tmp2 = $cf->getResultField("${cache}","writeMiss") 
      + $cf->getResultField("${cache}","writeHalfMiss")
      + $cf->getResultField("${cache}","writeHit")+1;
    printf "%5.1f%", (100*$tmp/$tmp2);

    my $tmp = $cf->getResultField("${cache}","busReadHit");
    my $tmp2 = $cf->getResultField("${cache}","busReadMiss") 
      + $cf->getResultField("${cache}","busReadHalfMiss")
      + $cf->getResultField("${cache}","busReadHit")+1;
    printf ",%5.1f%) ", (100*$tmp/$tmp2);


    if ($op_enPower eq "true") {

      my @cache_in_parts = split /[(,)]/, $cache;

      my $isCache = index(@cache_in_parts[0], "IL1");
      if ($isCache != -1){
        my $mypow = $cf->getResultField("pwrDynP(@cache_in_parts[1])_icache","v");
        printf " %8.0f " ,$mypow;
        $op_chippower += $mypow;
        $mypow = $cf->getResultField("pwrLkgP(@cache_in_parts[1])_icache","v");
        printf " %8.0f " ,$mypow;
        $op_chippower_L += $mypow;

      } else {
        $isCache = index(@cache_in_parts[0], "DL1");
        if ($isCache != -1){
          my $mypow = $cf->getResultField("pwrDynP(@cache_in_parts[1])_dcache","v");
          printf " %8.0f " ,$mypow;
          $op_chippower += $mypow;
          $mypow = $cf->getResultField("pwrLkgP(@cache_in_parts[1])_dcache","v");
          printf " %8.0f " ,$mypow;
          $op_chippower_L += $mypow;

        } else {

          #printf "DEBUG:[%s]", "pwrDyn@cache_in_parts[0](@cache_in_parts[1])";
          my $mypow = $cf->getResultField("pwrDyn@cache_in_parts[0](@cache_in_parts[1])","v");
          printf " %8.0f " ,$mypow;
          $op_chippower += $mypow;
          $mypow = $cf->getResultField("pwrLkg@cache_in_parts[0](@cache_in_parts[1])","v");
          printf " %8.0f " ,$mypow;
          $op_chippower_L += $mypow;

        }

      }
    } else {
    }
    printf "\n";
  }

}

sub getProcnInst {
  my $i = shift;

  my $nInst = $cf->getResultField("P(${i})", "nCommitted");

  return $nInst;
}

sub showStatReport {
  my $file = shift;

  printf "################################################################################\n";

  my $name = $file;
  $name =~ /.*sesc\_([^ ]*)......./;
  $name = $1;

  my $totCycles = 0;
  my $nCycles=0;
  for(my $i=0;$i<$nCPUs;$i++) {
    my $val = $cf->getResultField("P(${i})","clockTicks");
    $totCycles += $val;
    $nCycles = $val if ($nCycles < $val);
  }

  my $nInst;
  my $tmp;
  for(my $i=0;$i<$nCPUs;$i++) {
    $tmp += $cf->getResultField("P(${i})_FetchEngine","nDelayInst1");
    $tmp += $cf->getResultField("P(${i})_FetchEngine","nDelayInst2");
    $tmp += $cf->getResultField("P(${i})_FetchEngine","nDelayInst3");

    $nInst += getProcnInst($i);
  }
  my $nGradInsts  = $nInst;
  my $nWPathInsts = $tmp;

#############################################################################
  printf "#table0                            BusyCPU %: CommitInst: nCycles : nWPathInsts : IPC\n";

  printf "table0  %25s ", $name;
  # BusyCPU
  if($nCycles >0){
    printf " %9.2f ", 100*$totCycles/$nCycles;
  }
  else{
    printf "nCycles is zero!!!!!!!!";
  }
  # CommitInst
  printf " %9.0f ", $nGradInsts;
  # nCycles
  printf " %9.0f ", $nCycles;
  # of wrong path instructions
  printf " %9.0f ", $nWPathInsts;
  # IPC
  printf " %9.3f ", $nGradInsts/($nCycles+1);
  printf "\n";

#############################################################################
  if ($cf->getResultField("P(0)_robUsed","n")) {
    printf "#table8                              ProcId : ROBuse %: LDQ Use %: STQ use %: c1 winUse % : c2 winUse % :...\n";

    for(my $i=0;$i<$nCPUs;$i++) {
      my $cpuType = $cf->getConfigEntry(key=>"cpusimu",index=>$i);
      last unless (defined $cf->getConfigEntry(key=>"robSize",section=>$cpuType));

      printf "table8  %25s  %9d ", $name, $i;

      my $max;
      my $tmp;
      # ROBUse
      $max = $cf->getConfigEntry(key=>"robSize",section=>$cpuType);
      $tmp = $cf->getResultField("P(${i})_robUsed","v");
      printf " %9.2f ", 100*$tmp/$max;
      # LDQ Use
      $max = $cf->getConfigEntry(key=>"maxLoads",section=>$cpuType);
      $tmp = $cf->getResultField("P(${i})_FULoad_ldqNotUsed","v");
      printf " %9.2f ", 100*($max-$tmp)/($max+1);
      # STQ Use
      $max = $cf->getConfigEntry(key=>"maxStores",section=>$cpuType);
      $tmp = $cf->getResultField("P(${i})_FUStore_stqNotUsed","v");
      printf " %9.2f ", 100*($max-$tmp)/($max+1);
      printf "\n";
    }
  }

############################################################################# 
  for(my $i=0;$i<$nCPUs;$i++) {
    my $clockTicks= $cf->getResultField("P(${i})","clockTicks");
    next unless( $clockTicks > 1 );

    my @flist;
    if( $op_all ) {
      opendir(DIR,".");
      @flist = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
      closedir(DIR);
    }elsif( $op_last ) {
      opendir(DIR,".");
      my @tmp = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
      @flist = ($tmp[@tmp-1]);
      closedir(DIR);
    }else{
      @flist = @ARGV;
    }

    printf "#table18                 Benchmark           VPC_ACC:       VPC_FILTER_ACC:      FL2_ACC:          L2_ACC:        filename:\n";
    printf "table18 %26s ", $name;

    my $vpc_acc = $cf->getResultField("DL1(${i})","writeHit"); 
    $vpc_acc += $cf->getResultField("DL1(${i})","readHit"); 
    $vpc_acc += $cf->getResultField("DL1(${i})","writeMiss"); 
    $vpc_acc += $cf->getResultField("DL1(${i})","readHalfMiss"); 
    $vpc_acc += $cf->getResultField("DL1(${i})","writeHalfMiss"); 
    $vpc_acc += $cf->getResultField("DL1(${i})","readMiss"); 

    my $vpc_filter_acc = $cf->getResultField("P(${i})","wcbMiss"); 
    $vpc_filter_acc += $cf->getResultField("P(${i})","wcbHit"); 

    my $fl2_acc = $cf->getResultField("FL2","writeHit"); 
    $fl2_acc += $cf->getResultField("FL2","readHit"); 
    $fl2_acc += $cf->getResultField("FL2","writeMiss"); 
    $fl2_acc += $cf->getResultField("FL2","readHalfMiss"); 
    $fl2_acc += $cf->getResultField("FL2","writeHalfMiss"); 
    $fl2_acc += $cf->getResultField("FL2","readMiss"); 

    my $l2_acc = $cf->getResultField("L2(${i})","writeHit"); 
    $l2_acc += $cf->getResultField("L2(${i})","readHit"); 
    $l2_acc += $cf->getResultField("L2(${i})","writeMiss"); 
    $l2_acc += $cf->getResultField("L2(${i})","readHalfMiss"); 
    $l2_acc += $cf->getResultField("L2(${i})","writeHalfMiss"); 
    $l2_acc += $cf->getResultField("L2(${i})","readMiss"); 

    printf "     %12d", $vpc_acc;
    printf "         %12d", $vpc_filter_acc;
    printf "   %12d", $fl2_acc;
    printf "     %12d", $l2_acc;
    printf "        %-20s\n ", $file;
    print "\n";
  }

#############################################################################
  for(my $i=0;$i<$nCPUs;$i++) {
    my $clockTicks= $cf->getResultField("P(${i})","clockTicks");
    next unless( $clockTicks > 1 );

    printf "#table17 Benchmark:             L2Miss:  L1Miss:   L1Hit:            filename:\n";
    printf "table17 %22s ", $name;



    my $l2_miss   = ($cf->getResultField("L2", "readMiss")) + ($cf->getResultField("L2", "readHalfMiss")) + ($cf->getResultField("L2", "writeMiss")) + ($cf->getResultField("L2", "writeHalfMiss")); 

    my $l1_miss   = ($cf->getResultField("DL1(${i})", "readMiss")) + ($cf->getResultField("DL1(${i})", "readHalfMiss")) + ($cf->getResultField("DL1(${i})", "writeMiss")) + ($cf->getResultField("DL1(${i})", "writeHalfMiss")); 
    my $l1_hit    = ($cf->getResultField("DL1(${i})", "readHit")) +  ($cf->getResultField("DL1(${i})", "writeHit"));

    printf "   %d ", $l2_miss;
    printf "   %d ", $l1_miss;
    printf " %d ", $l1_hit;

    printf " %-20s\n ", $file;          
    print "\n";
  }

############################################################################# 
  for(my $i=0;$i<$nCPUs;$i++) {
    my $clockTicks= $cf->getResultField("P(${i})","clockTicks");
    next unless( $clockTicks > 1 );

    my @flist;
    if( $op_all ) {
      opendir(DIR,".");
      # short by modification date
      @flist = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
      closedir(DIR);
    }elsif( $op_last ) {
      opendir(DIR,".");
      # short by modification date
      my @tmp = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
      @flist = ($tmp[@tmp-1]);
      closedir(DIR);
    }else{
      @flist = @ARGV;
    }

    printf "#table13                  Benchmark      IPC:       PercentInstReplay:     PercentWriteBacks:           filename:\n";
    printf "table13 %26s ", $name;


    # IPC 
    my $nInst = getProcnInst($i);
    printf "%9.3f ", $nInst/$clockTicks;

    # Inst/Replay
    my $nReplayInst_n = $cf->getResultField("P(${i})_nReplayInst","n"); 
    if ($nReplayInst_n==0) {
      $nReplayInst_n += $cf->getResultField("P(${i})_specld","stldViolations"); 
    }

    # FIXME: The iBALU, iCALU should not be hardcoded. the shared.conf can
    # control the unit names and operations per unit

    my $iAALU = $cf->getResultField("P(${i})_ExeEngine_iRALU","n")
      + $cf->getResultField("P(${i})_ExeEngine_iAALU","n");

    my $iBALU  = $cf->getResultField("P(${i})_ExeEngine_iBALU_RBRANCH","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_LBRANCH","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_RJUMP","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_LJUMP","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_RCALL","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_LCALL","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_RET","n");

    my $iLALU = $cf->getResultField("P(${i})_ExeEngine_iLALU_LD","n");

    my $iSALU = $cf->getResultField("P(${i})_ExeEngine_iSALU_ST","n")
      + $cf->getResultField("P(${i})_ExeEngine_iSALU_LL","n")
      + $cf->getResultField("P(${i})_ExeEngine_iSALU_SC","n")
      + $cf->getResultField("P(${i})_ExeEngine_iSALU_ADDR","n");

    my $iCALU = $cf->getResultField("P(${i})_ExeEngine_iCALU_FPMULT","n")
      + $cf->getResultField("P(${i})_ExeEngine_iCALU_FPDIV","n")
      + $cf->getResultField("P(${i})_ExeEngine_iCALU_FPALU","n")
      + $cf->getResultField("P(${i})_ExeEngine_iCALU_MULT","n")
      + $cf->getResultField("P(${i})_ExeEngine_iCALU_DIV","n");

    my $nInst_real = $iAALU + $iBALU + $iCALU + $iSALU + $iLALU;


    if ($nReplayInst_n) {
      my $nComm = getProcnInst($i);
      printf "                  %6.3f%%",100*($nInst_real-$nComm)/$nInst_real;
    }

    my $writeHit      = $cf->getResultField("DL1(${i})","writeHit");
    my $writeExcl     = $cf->getResultField("DL1(${i})","writeExclusive");
    my $writeMiss     = $cf->getResultField("DL1(${i})","writeMiss");
    my $writeHalfMiss = $cf->getResultField("DL1(${i})","writeHalfMiss");
    my $writeBack     = $cf->getResultField("DL1(${i})","writeBack");

    my $readHit      = $cf->getResultField("DL1(${i})","readHit");
    my $readHalfMiss = $cf->getResultField("DL1(${i})","readHalfMiss");
    my $readMiss     = $cf->getResultField("DL1(${i})","readMiss");

    my $divisor = $writeHit + $writeExcl + $writeMiss + $writeHalfMiss + $writeBack + $readHit + $readHalfMiss + $readMiss;
    if($divisor==0){
      $divisor = 1;
    }
    my $writeBack_ratio = $writeBack/($divisor);

    printf "                %6.3f%%", ($writeBack_ratio*100);
    printf "           %-20s\n ", $file;
    print "\n";
  }

#############################################################################  
  for(my $i=0;$i<$nCPUs;$i++) {
    my $clockTicks= $cf->getResultField("P(${i})","clockTicks");
    next unless( $clockTicks > 1 );

    my @flist;
    if( $op_all ) {
      opendir(DIR,".");
      # short by modification date
      @flist = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
      closedir(DIR);
    }elsif( $op_last ) {
      opendir(DIR,".");
      # short by modification date
      my @tmp = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
      @flist = ($tmp[@tmp-1]);
      closedir(DIR);
    }else{
      @flist = @ARGV;
    }

    printf "#table15                Benchmark:      nFetched:   nCommitted:       Ratio:            filename:\n";
    printf "table15 %26s ", $name;
    foreach my $file (@flist) {
      my $committed = ($cf->getResultField("P(${i})", "nCommitted"));
      my $fetched  = ($cf->getResultField("P(${i})_FetchEngine", "nFetched"));
      my $ratio    = $fetched/$committed;

      printf "     %d ", $fetched;
      printf "     %d ", $committed;
      printf "     %f ", $ratio;
      printf "           %-20s\n ", $file;
    }
    print "\n";
  }

  ############################################################################# 

  my $progressedTime = $cf->getResultField("progressedTime", "max");

  my @flist;
  if( $op_all ) {
    opendir(DIR,".");
    # short by modification date
    @flist = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
    closedir(DIR);
  }elsif( $op_last ) {
    opendir(DIR,".");
    # short by modification date
    my @tmp = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
    @flist = ($tmp[@tmp-1]);
    closedir(DIR);
  }else{
    @flist = @ARGV;
  }

  printf "#table23     Progressed Time:\tBenchmark:\t\t\t\t\t   filename:\n";
  printf "table23     %12e\t%-50s %-50s", $progressedTime, $name, $file;
  print "\n";

  #############################################################################
  for(my $i=0;$i<$nCPUs;$i++) {
    my $clockTicks= $cf->getResultField("P(${i})","clockTicks");
    next unless( $clockTicks > 1 );

    my @flist;
    if( $op_all ) {
      opendir(DIR,".");
      # short by modification date
      @flist = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
      closedir(DIR);
    }elsif( $op_last ) {
      opendir(DIR,".");
      # short by modification date
      my @tmp = sort {(stat($a))[9] cmp (stat($b))[9]} (grep (/^esesc\_.+\..{6}$/, readdir (DIR)));
      @flist = ($tmp[@tmp-1]);
      closedir(DIR);
    }else{
      @flist = @ARGV;
    }

    printf "#table46                Benchmark:      WBPASS:             WBFAIL:      VPC_RDHIT:      VPC_RDMISS:       VPC_HALFRDMISS:      VPC_WRHIT:      VPC_WRMISS        VPC_PASS        VPC_FAIL    filename:\n";
    printf "table46 %26s ", $name;
    foreach my $file (@flist) {
      my $wbpass         = ($cf->getResultField("DL1(${i})", "wcbPass"));
      my $wbfail         = ($cf->getResultField("DL1(${i})", "wcbFail"));
      my $vpc_rdhit      = ($cf->getResultField("DL1(${i})", "readHit"));              
      my $vpcpass        = ($cf->getResultField("DL1(${i})", "vpcPass"));
      my $vpcfail        = ($cf->getResultField("DL1(${i})", "vpcFail"));
      my $vpc_rdmiss     = ($cf->getResultField("DL1(${i})", "readMiss"));
      my $vpc_rdhalfmiss = ($cf->getResultField("DL1(${i})", "readHalfMiss"));
      my $vpc_writehit   = ($cf->getResultField("DL1(${i})", "writeHit"));
      my $vpc_writemiss  = ($cf->getResultField("DL1(${i})", "writeMiss"));              

      printf "     %10d ", $wbpass; 
      printf "     %10d ", $wbfail;
      printf "     %10d ", $vpc_rdhit; 
      printf "      %10d ", $vpc_rdmiss;
      printf "           %10d ", $vpc_rdhalfmiss;
      printf "     %10d ", $vpc_writehit; 
      printf "     %10d ", $vpc_writemiss;             
      printf "     %10d ", $vpcpass;             
      printf "     %10d ", $vpcfail;             
      printf "     %-20s\n ", $file;
    }
    print "\n";
  }

############################################################################# 

  printf "\n\n";
  # HACK, check if ROB power exists, if not, then power model wasn't on, don't print this table
  my $pwr_check = $cf->getResultField("maxpwr_ROB","max");  
    if($pwr_check > 0){
      my $cpuType = $cf->getConfigEntry(key=>"cpusimu",index=>0);
      foreach my $file (@flist) {

        if($cpuType eq "scooreCORE"){
          printf "#table14                 Benchmark:      ROB:      FL2:   VPC-WB:      VPC:     SPECBUFF:      TLB:      Mem_Hier:        Rest:      Instructions:      Clockticks:            filename:\n"
        }else {
          printf "#table14                 Benchmark:      ROB:   DCACHE:      LSU:     TLB:      Mem_Hier:        Rest:      Instructions:      Clockticks:     StoreSet:      filename:\n"
        } 
        printf "table14 %27s ", $name;                  
      }

      for(my $i=0;$i<$nCPUs;$i++) {
        my $cpuType = $cf->getConfigEntry(key=>"cpusimu",index=>$i);
        my $clockTicks= $cf->getResultField("P(${i})","clockTicks");
        next unless( $clockTicks > 1 );
        printf("table14 ");
        my $nInst = ($cf->getResultField("P(${i})", "nCommitted"));

        # ROB Power 
        my $rob_pwr = $cf->getResultField("pwrDynP(${i})_ROB","v");
        printf "%9.3f ", $clockTicks*$rob_pwr/$nInst;

        # dcache or FL2 Power
        if($cpuType eq "scooreCORE"){
          my $VPCfilter_pwr = $cf->getResultField("pwrDynFL2(${i})","v");
          printf "%9.3f ", $clockTicks*$VPCfilter_pwr/$nInst;
        }else{
          my $dcache_pwr = $cf->getResultField("pwrDynP(${i})_dcache","v");
          my $dcache_maxpwr = $cf->getResultField("maxpwr_dcacheWBB","max")+
            $cf->getResultField("maxpwr_dcacheFillBuffer","max")+
            $cf->getResultField("maxpwr_dcache","max")+
            $cf->getResultField("maxpwr_dcacheMissBuffer","max");

          my $tlb_maxpwr      = $cf->getResultField("maxpwr_DTLB","max");
          my $tlb_ratio       = $tlb_maxpwr / ($dcache_maxpwr + $tlb_maxpwr);
          my $dcache_true_pwr = $dcache_pwr * (1-$tlb_ratio);
          printf "%9.3f ", $clockTicks*$dcache_true_pwr/$nInst;
        }

# VPC-WB
        my $VPCSPECBUFF_true_pwr = -1;
        if($cpuType eq "scooreCORE"){
          my $VPCWB_maxpwr       = $cf->getResultField("maxpwr_VPC_buffer","max");
          my $VPCSPECBR_maxpwr   = $cf->getResultField("maxpwr_VPC_SpecBR","max");
          my $VPCSPECBW_maxpwr   = $cf->getResultField("maxpwr_VPC_SpecBW","max"); 
          my $VPC_tot_max        = $VPCSPECBW_maxpwr + $VPCSPECBR_maxpwr + $VPCWB_maxpwr+1;
          my $LSU_pwr            = $cf->getResultField("pwrDynP(${i})_LSU","v");
          my $VPCWB_ratio        = $VPCWB_maxpwr/$VPC_tot_max;
          my $VPCWB_true_pwr     = $VPCWB_ratio * $LSU_pwr;
          my $VPCSPECBUFF_ratio  = ($VPCSPECBW_maxpwr + $VPCSPECBR_maxpwr)/$VPC_tot_max;
          $VPCSPECBUFF_true_pwr  = $VPCSPECBUFF_ratio * $LSU_pwr; 
          printf "%9.3f ", $clockTicks*$VPCWB_true_pwr/$nInst;             
        }

        my $lsq_pwr;
        my $lfst_max_pwr;
        my $ssit_max_pwr;
        my $loadQ_max_pwr;
        my $LSQ_max_pwr;

        my $ss_ratio;
        my $ss_pwr;
        my $true_lsq_pwr;

        if($cpuType eq "tradCORE"){
          $lsq_pwr = $cf->getResultField("pwrDynP(${i})_LSU","v");
          $lfst_max_pwr  = $cf->getResultField("maxpwr_LFST","max");
          $ssit_max_pwr  = $cf->getResultField("maxpwr_SSIT","max");          
          $loadQ_max_pwr = $cf->getResultField("maxpwr_LoadQueue","max");          
          $LSQ_max_pwr   = $cf->getResultField("maxpwr_LSQ","max");          
          $ss_ratio = ($lfst_max_pwr + $ssit_max_pwr)/($lfst_max_pwr + $ssit_max_pwr + $loadQ_max_pwr + $LSQ_max_pwr);
          $ss_pwr = $ss_ratio * $lsq_pwr;
          $true_lsq_pwr = $lsq_pwr - $ss_pwr;
        }

        #LSQ or VPC Power
        if($cpuType eq "scooreCORE"){
          my $VPC_pwr = $cf->getResultField("pwrDynP(${i})_dcache","v");
          printf "%9.3f ", $clockTicks*$VPC_pwr/$nInst;
        }else{
          printf "%9.3f ", $clockTicks*$true_lsq_pwr/$nInst;
        }

        #SPECBUFF Power
        if($cpuType eq "scooreCORE"){
          printf "    %9.3f ", $clockTicks*$VPCSPECBUFF_true_pwr/$nInst;
        }

        #TLB Power
        my $tlb_maxpwr = $cf->getResultField("maxpwr_DTLB","max");
        my $dcache_maxpwr = $cf->getResultField("maxpwr_dcacheWBB","max")+
          $cf->getResultField("maxpwr_dcacheFillBuffer","max")+
          $cf->getResultField("maxpwr_dcache","max")+
          $cf->getResultField("maxpwr_dcacheMissBuffer","max");


        if($dcache_maxpwr){
          my $tlb_ratio = $tlb_maxpwr / ($dcache_maxpwr + $tlb_maxpwr);
          my $dcache_pwr = $cf->getResultField("pwrDynP(${i})_dcache","v");
          my $tlb_pwr = $dcache_pwr * $tlb_ratio;
          printf "%9.3f   ", $clockTicks*$tlb_pwr/$nInst;  
        }

        #Rest of Mem Hierarchy Power
        my $mem_hier_pwr = $cf->getResultField("pwrDynL3(${i})","v")+
          $cf->getResultField("pwrDynL2(${i})","v")+
          $cf->getResultField("pwrDynMemBus(${i})","v");
        printf "   %9.3f    ", $clockTicks*$mem_hier_pwr/$nInst;  

        #Rest of the Processor
        my $rest_of_proc_pwr = $cf->getResultField("pwrP(${i})_EXE","v")+
          $cf->getResultField("pwrDynP(${i})_fetch","v")+
          $cf->getResultField("pwrDynP(${i})_RNU","v")+
          $cf->getResultField("pwrDynP(${i})_icache","v");
        printf "%9.3f ", $clockTicks*$rest_of_proc_pwr/$nInst;
        printf "          %d         %d ", $nInst, $clockTicks;
 
        #StoreSets
        if($cpuType eq "tradCORE"){
          printf "%9.3f ", $clockTicks*$ss_pwr/$nInst;      
        }
        printf "           %-20s\n ", $file;
        print "\n\n";
      }
    }

############################################################################# 
  printf "\n\n";
  #HACK, check if ROB power exists, if not, then power model wasn't on, don't print this table 
  my $pwr_check = $cf->getResultField("maxpwr_ROB","max"); 
    if($pwr_check > 0){
      my $cpuType = $cf->getConfigEntry(key=>"cpusimu",index=>0);
      foreach my $file (@flist) {

        if($cpuType eq "scooreCORE"){
          printf "#table47 Benchmark:               ROB:       FL2:      VPC-WB:     VPC:        SPECBUFF:    TLB:      Mem_Hier:        Rest:       Instructions:      filename:\n"
        }else {
          printf "#table47 Benchmark:               ROB:     DCACHE:    LSU:          TLB:      Mem_Hier:        Rest:       Instructions:     StoreSet:  filename:\n"
        } 
        my @namesplit;
        @namesplit = split('_', $name);
        printf "table47  %-21s ", $namesplit[0];                  
      }

      my $rob_pwr_sum = 0;
      my $VPCfilter_pwr_sum = 0;
      my $dcache_true_pwr_sum = 0;
      my $VPCWB_true_pwr_sum = 0;
      my $VPC_pwr_sum = 0;
      my $true_lsq_pwr_sum = 0;
      my $VPCSPECBUFF_true_pwr_sum = 0;
      my $tlb_pwr_sum = 0;
      my $mem_hier_pwr_sum = 0;
      my $rest_of_proc_pwr_sum = 0;
      my $ss_pwr_sum = 0;
      my $nInst_sum = 0;

      for(my $i=0;$i<$nCPUs;$i++) {
        my $cpuType = $cf->getConfigEntry(key=>"cpusimu",index=>$i);
        my $clockTicks= $cf->getResultField("P(${i})","clockTicks");
        my $nInst = ($cf->getResultField("P(${i})", "nCommitted"));
        $nInst_sum += $nInst;

        # ROB Power 
        my $rob_pwr = $cf->getResultField("energyP(${i})_ROB","v");
        $rob_pwr_sum += $rob_pwr;

        # dcache or FL2 Power
        if($cpuType eq "scooreCORE"){
          my $VPCfilter_pwr = $cf->getResultField("energyFL2(${i})","v");
          $VPCfilter_pwr_sum += $VPCfilter_pwr;
        }else{
          my $dcache_pwr = $cf->getResultField("energyP(${i})_dcache","v");
          my $dcache_maxpwr = $cf->getResultField("maxpwr_dcacheWBB","max")+
            $cf->getResultField("maxpwr_dcacheFillBuffer","max")+
            $cf->getResultField("maxpwr_dcache","max")+
            $cf->getResultField("maxpwr_dcacheMissBuffer","max");

          my $tlb_maxpwr      = $cf->getResultField("maxpwr_DTLB","max");
          my $tlb_ratio       = $tlb_maxpwr / ($dcache_maxpwr + $tlb_maxpwr);
          my $dcache_true_pwr = $dcache_pwr * (1-$tlb_ratio);
          $dcache_true_pwr_sum += $dcache_true_pwr;
        }

        # VPC-WB
        my $VPCSPECBUFF_true_pwr = -1;
        if($cpuType eq "scooreCORE"){
          my $VPCWB_maxpwr       = $cf->getResultField("maxpwr_VPC_buffer","max");
          my $VPCSPECBR_maxpwr   = $cf->getResultField("maxpwr_VPC_SpecBR","max");
          my $VPCSPECBW_maxpwr   = $cf->getResultField("maxpwr_VPC_SpecBW","max"); 
          my $VPC_tot_max        = $VPCSPECBW_maxpwr + $VPCSPECBR_maxpwr + $VPCWB_maxpwr+1;
          my $LSU_pwr            = $cf->getResultField("energyP(${i})_LSU","v");
          my $VPCWB_ratio        = $VPCWB_maxpwr/$VPC_tot_max;
          my $VPCWB_true_pwr     = $VPCWB_ratio * $LSU_pwr;
          $VPCWB_true_pwr_sum   += $VPCWB_true_pwr;
          my $VPCSPECBUFF_ratio  = ($VPCSPECBW_maxpwr + $VPCSPECBR_maxpwr)/$VPC_tot_max;
          $VPCSPECBUFF_true_pwr  = $VPCSPECBUFF_ratio * $LSU_pwr; 
          $VPCSPECBUFF_true_pwr_sum += $VPCSPECBUFF_true_pwr;
        }

        my $lsq_pwr;
        my $lfst_max_pwr;
        my $ssit_max_pwr;
        my $loadQ_max_pwr;
        my $LSQ_max_pwr;

        my $ss_ratio;
        my $ss_pwr;
        my $true_lsq_pwr;

        if($cpuType eq "tradCORE"){
          $lsq_pwr = $cf->getResultField("energyP(${i})_LSU","v");
          $lfst_max_pwr  = $cf->getResultField("maxpwr_LFST","max");
          $ssit_max_pwr  = $cf->getResultField("maxpwr_SSIT","max");          
          $loadQ_max_pwr = $cf->getResultField("maxpwr_LoadQueue","max");          
          $LSQ_max_pwr   = $cf->getResultField("maxpwr_LSQ","max");          
          $ss_ratio = ($lfst_max_pwr + $ssit_max_pwr)/($lfst_max_pwr + $ssit_max_pwr + $loadQ_max_pwr + $LSQ_max_pwr);
          $ss_pwr = $ss_ratio * $lsq_pwr;
          $ss_pwr_sum += $ss_pwr;
          $true_lsq_pwr = $lsq_pwr - $ss_pwr;
          $true_lsq_pwr_sum += $true_lsq_pwr;
        }

        #LSQ or VPC Power
        if($cpuType eq "scooreCORE"){
          my $VPC_pwr = $cf->getResultField("energyP(${i})_dcache","v");
          $VPC_pwr_sum +=$VPC_pwr;
        }

        #TLB Power
        my $tlb_maxpwr = $cf->getResultField("maxpwr_DTLB","max");
        my $dcache_maxpwr = $cf->getResultField("maxpwr_dcacheWBB","max")+
          $cf->getResultField("maxpwr_dcacheFillBuffer","max")+
          $cf->getResultField("maxpwr_dcache","max")+
          $cf->getResultField("maxpwr_dcacheMissBuffer","max");


        if($dcache_maxpwr){
          my $tlb_ratio = $tlb_maxpwr / ($dcache_maxpwr + $tlb_maxpwr);
          my $dcache_pwr = $cf->getResultField("energyP(${i})_dcache","v");
          my $tlb_pwr = $dcache_pwr * $tlb_ratio;
          $tlb_pwr_sum += $tlb_pwr;
        }

        #Rest of Mem Hierarchy Power
        my $L3ener = $cf->getResultField("energyL3(${i})","v");
        my $L2ener = $cf->getResultField("energyL2(${i})","v");
        my $MemBusener = $cf->getResultField("energyMemBus(${i})","v");

        my $mem_hier_pwr = $L3ener + $L2ener + $MemBusener;

        $mem_hier_pwr_sum += $mem_hier_pwr;

        #Rest of the Processor
        my $rest_of_proc_pwr = $cf->getResultField("energyP(${i})_EXE","v")+
          $cf->getResultField("energyP(${i})_fetch","v")+
          $cf->getResultField("energyP(${i})_RNU","v")+
          $cf->getResultField("energyP(${i})_icache","v");
        $rest_of_proc_pwr_sum += $rest_of_proc_pwr;
      }

      printf "%9.3f ", $rob_pwr_sum;
      if($cpuType eq "scooreCORE"){
        printf "%9.3f ", $VPCfilter_pwr_sum;
      }else{
        printf "%9.3f ", $dcache_true_pwr_sum;
      }
      if($cpuType eq "scooreCORE"){
        printf "%9.3f ", $VPCWB_true_pwr_sum;
        printf "   %9.3f ", $VPC_pwr_sum;
      }
      else{
        printf "%9.3f ", $true_lsq_pwr_sum;
      }

      #SPECBUFF Power                                                                                                  
      if($cpuType eq "scooreCORE"){
        printf " %9.3f ", $VPCSPECBUFF_true_pwr_sum;
      }
      printf "   %9.3f   ", $tlb_pwr_sum;
      printf "  %9.3f    ", $mem_hier_pwr_sum;
      printf "%9.3f ", $rest_of_proc_pwr_sum;
      printf "          %d ", $nInst_sum;
      if($cpuType eq "tradCORE"){
        printf "%9.3f ", $ss_pwr_sum;
      }

      printf "     %-24s\n ", $file;
      print "\n\n";

    }

#############################################################################

  for(my $i=0;$i<$nCPUs;$i++) {
    my $clockTicks= $cf->getResultField("P(${i})","clockTicks");
    next unless( $clockTicks > 1 );

    printf "#table9a                            IPC :  uIPC : szFB : szBB : brMiss : brMissTime : iMissRate \n";
    printf "table9a  %26s ", $name;

    $nInst = getProcnInst($i);

    my $rawInst = $cf->getResultField("Reader(${i})","rawInst");

    # IPC
    printf " %9.3f ", $rawInst/$clockTicks;

    # uIPC
    printf " %9.3f ", $nInst/$clockTicks;

    # szFB
    my $nTaken    = $cf->getResultField("P(${i})_BPred","nTaken");
    printf " %9.2f ", $nInst/($nTaken+1);

    # szBB
    my $nBranches = $cf->getResultField("P(${i})_BPred","nBranches");
    printf " %9.2f ", $nInst/($nBranches+1);

    # branchMissRate
    my $nMiss = $cf->getResultField("P(${i})_BPred ","nMiss");
    printf " %9.2f ", 100*$nMiss/($nBranches+1);

    # brMissTime
    printf " %9.2f ", $cf->getResultField("P(${i})_FetchEngine_avgBranchTime","v");

    # icache miss
    my $iaccess = $cf->getResultField("P(${i})_IL1","readHalfMiss") + 
      $cf->getResultField("P(${i})_IL1","readMiss") + 
      $cf->getResultField("P(${i})_IL1","readHit") + 
      $cf->getResultField("P(${i})_IL1","writeHalfMiss") + 
      $cf->getResultField("P(${i})_IL1","writeMiss") + 
      $cf->getResultField("P(${i})_IL1","writeHit");

    $iaccess = 1 unless ($iaccess);

    printf " %9.2f ", 100*($cf->getResultField("P(${i})_IL1","readMiss")+$cf->getResultField("P(${i})_IL1","writeMiss")) /$iaccess;

    printf "\n";

    my $nDepsOverflow = 1;
    my $nDeps_0;
    my $nDeps_1;
    my $nDeps_2;
    my $nDepsMiss;

    my $cpuType = $cf->getConfigEntry(key=>"cpusimu",index=>$i);
    for (my $j=0; ; $j++) {
      my $clusterType = $cf->getConfigEntry(key=>"cluster", section=>$cpuType, index=>$j);
      last unless (defined $clusterType);

      $nDepsOverflow += $cf->getResultField("P(${i})_${clusterType}_depTable","nDepsOverflow");

      $nDeps_0   += $cf->getResultField("P(${i})_${clusterType}_depTable","nDeps_0");
      $nDeps_1   += $cf->getResultField("P(${i})_${clusterType}_depTable","nDeps_1");
      $nDeps_2   += $cf->getResultField("P(${i})_${clusterType}_depTable","nDeps_2");
      $nDepsMiss += $cf->getResultField("P(${i})_${clusterType}_depTable","nDepsMiss");
    }

    next unless ($nDeps_0 > 1);

    printf "#table9                              ProcID: IPC : 0 deps : 1 deps : 2 deps : %re-dispatcch :#Overflows :L1 hit Predictor Accuracy\n";
    # ProcID
    printf "table9  %26s  %9d : ", $name, $i;


    my $bench = $name;
    $bench =~ /([^ _]*).mips$/;
    $bench = $1;

    printf " %-10s ", $bench;

    # IPC
    printf " & %9.3f ", $nInst/$clockTicks;

    # nDeps
    printf " & %9.1f ", 100*$nDeps_0/($nDeps_0 + $nDeps_1 + $nDeps_2 + 1);
    printf " & %9.1f ", 100*$nDeps_1/($nDeps_0 + $nDeps_1 + $nDeps_2 + 1);
    printf " & %9.1f ", 100*$nDeps_2/($nDeps_0 + $nDeps_1 + $nDeps_2 + 1);

    # re-dispatch
    printf " & %9.1f ", 100*$nDepsMiss/($nDeps_0 + $nDeps_1 + $nDeps_2 + 1);

    # cycles between overflows
    if ($clockTicks/($nDepsOverflow+1) > 50e3) {
      printf " & \$>\$50k ";
    }else{
      printf " & %9.0f ", $clockTicks/($nDepsOverflow+1);
    }

    print "\\\\ :";

    printf " %-10s ", $bench;

    my $daccess = $cf->getResultField("P(${i})_DL1","readHalfMiss") + 
      $cf->getResultField("P(${i})_DL1","readMiss") + 
      $cf->getResultField("P(${i})_DL1","readHit") + 
      $cf->getResultField("P(${i})_DL1","writeHalfMiss") + 
      $cf->getResultField("P(${i})_DL1","writeMiss") + 
      $cf->getResultField("P(${i})_DL1","writeHit")+
      $cf->getResultField("P(${i})_DL1","avgMemLat","v","n")+
      $cf->getResultField("P(${i})_DL1","invalidateHit")+
      $cf->getResultField("P(${i})_DL1","MSHR0(${i})_wrhit")+
      $cf->getResultField("P(${i})_DL1","rd0_occ","v","n")+
      $cf->getResultField("P(${i})_DL1","bk0_occ","v","n")+
      $cf->getResultField("P(${i})_DL1","invalidateMiss")+
      $cf->getResultField("P(${i})_DL1","MSHR0(${i})_wrmiss")+
      $cf->getResultField("P(${i})_DL1","MSHR0_MSHR","nStallConflit")+
      $cf->getResultField("P(${i})_DL1",":writeBack")+
      $cf->getResultField("P(${i})_DL1",":lineFill")+
      $cf->getResultField("P(${i})_DL1","wr0_occ","v","n")+
      $cf->getResultField("P(${i})_DL1","busReadHit")+
      $cf->getResultField("P(${i})_DL1","busReadHalfMiss")+
      $cf->getResultField("P(${i})_DL1","MSHR0(${i})_rdhit")+
      $cf->getResultField("P(${i})_DL1","MSHR0(${i})_rdmiss")+
      $cf->getResultField("P(${i})_DL1","writeExclusive")+
      $cf->getResultField("P(${i})_DL1","ll0_occ","v","n")+
      $cf->getResultField("P(${i})_DL1","MSHR0_MSHR:nOverflows")+
      $cf->getResultField("P(${i})_DL1","MSHR0_MSHR:nUse")+
      $cf->getResultField("P(${i})_DL1",":busReadMiss");

    printf " & %9.2f ", 100*($cf->getResultField("P(${i})_DL1","readMiss")
        +$cf->getResultField("P(${i})_DL1","writeMiss")
        )/$daccess;

    my $nL1Hit_pHit   = $cf->getResultField("L1Pred","nL1Hit_pHit");
    my $nL1Hit_pMiss  = $cf->getResultField("L1Pred","nL1Hit_pMiss");
    my $nL1Miss_pHit  = $cf->getResultField("L1Pred","nL1Miss_pHit");
    my $nL1Miss_pMiss = $cf->getResultField("L1Pred","nL1Miss_pMiss");

    my $total = $nL1Miss_pMiss+$nL1Miss_pHit+$nL1Hit_pMiss+$nL1Hit_pHit+1;

    printf " & %9.2f ", 100*($nL1Miss_pMiss+$nL1Hit_pHit)/$total;
    printf " & %9.2f ", 100*$nL1Hit_pMiss /$total;
    printf " & %9.2f ", 100*$nL1Miss_pHit /$total;

    print "\\\\";

    print "\n";
  }
}

sub dumpKIPS{
  my $file = shift;

  # Begin Global Stats
  $cpuType = $cf->getConfigEntry(key=>"cpusimu");

  my $techSec = $cf->getConfigEntry(key=>"technology");

  $freq = $cf->getConfigEntry(key=>"frequency",section=>$techSec) / 1e6;
  # Old configuration type
  $freq = $cf->getConfigEntry(key=>"frequency",section=>$cpuType) / 1e6 unless($freq);
  $freq = 1e3 unless($freq);

  $nCPUs= $cf->getResultField("OSSim","nCPUs");
  unless( defined $nCPUs ) {
    print "Configuration file [$file] has a problem\n";
    next;
  }

  next unless ($cf->getResultField("OSSim","msecs"));

  for (my $j=0; ; $j++) {
    my $clusterType = $cf->getConfigEntry(key=>"cluster", section=>$cpuType, index=>$j);
    last unless (defined $clusterType);
    next unless ($cf->getResultField("P(0)_${clusterType}_depTable","nDepsOverflow") > 1);

    my $clk = 10*$cf->getResultField("P(0)_${clusterType}_depTable","nDepsOverflow");
    my $clockTicks = $cf->getResultField("P(0)","clockTicks");
    $slowdown = $clockTicks/($clockTicks+$clk);

    die "Must compute overflow in a different way" if ($nCPUs != 1);
  }
  $slowdown = 1 unless (defined $slowdown);

  $nLoadTotal  = 0;
  $nStoreTotal = 0;
  $nInstTotal  = 0;
  for(my $i=0;$i<$nCPUs;$i++) {
    $nLoadTotal  += $cf->getResultField("P(${i})_ExeEngine_iLALU_LD","n");
    $nStoreTotal += $cf->getResultField("P(${i})_ExeEngine_iSALU_ST","n");

    $nInstTotal += getProcnInst($i);
  }

  # End Global Stats
  my $name = $file;
  $name =~ /.*sesc\_([^ ]*)......./;
  $name = $1;
  printf "#table12               Rabbit Warmup Detail Timing Total KIPS \n";

  my $secs    = $cf->getResultField("OSSim","msecs");
  my $secs_gpu    = $cf->getResultField("OSSim","msecs_gpu");
  $secs_gpu = 1 if ($secs_gpu == 0);
  my $nSampler    = $cf->getResultField("OSSim","nSampler");

  $secs = 1 if( $secs == 0 );

  for(my $s=0; $s<$nSampler;$s++) {
    my $inst_t = 0;
    my $usec_t = 1;

    printf "table12    %s S(%d) KIPS ",$name, $s;
    foreach my $mode ("Rabbit", "Warmup", "Detail", "Timing") {
      my $inst = $cf->getResultField("S(${s})","${mode}Inst");
      my $usec = $cf->getResultField("S(${s})","${mode}Time");
      $inst_t = $inst_t + $inst;
      $usec_t = $usec_t + $usec;

      if ($usec == 0) {
        printf "     N/A";
      }else{
        printf " %5.0f ",($inst/($usec/1000));
      }
    }
    printf " %5.0f ",($inst_t/($usec_t/1000));
    print "\n";
  }
  print "\n";
}

sub simStats {
  my $file = shift;

  # Begin Global Stats
  $cpuType = $cf->getConfigEntry(key=>"cpusimu");
  my $techSec = $cf->getConfigEntry(key=>"technology");
  $freq = $cf->getConfigEntry(key=>"frequency",section=>$techSec) / 1e6;

  # Old configuration type
  $freq = $cf->getConfigEntry(key=>"frequency",section=>$cpuType) / 1e6 unless($freq);
  $freq = 1e3 unless($freq);

  $nCPUs= $cf->getResultField("OSSim","nCPUs");

  unless( defined $nCPUs ) {
    print "Configuration file [$file] has a problem\n";
    next;
  }

  next unless ($cf->getResultField("OSSim","msecs"));

  for (my $j=0; ; $j++) {
    my $clusterType = $cf->getConfigEntry(key=>"cluster", section=>$cpuType, index=>$j);
    last unless (defined $clusterType);
    next unless ($cf->getResultField("P(0)_${clusterType}_depTable","nDepsOverflow") > 1);

    my $clk = 10*$cf->getResultField("P(0)_${clusterType}_depTable","nDepsOverflow");
    my $clockTicks = $cf->getResultField("P(0)","clockTicks");
    $slowdown = $clockTicks/($clockTicks+$clk);

    die "Must compute overflow in a different way" if ($nCPUs != 1);
  }
  $slowdown = 1 unless (defined $slowdown);

  $nLoadTotal  = 0;
  $nStoreTotal = 0;
  $nInstTotal  = 0;
  for(my $i=0;$i<$nCPUs;$i++) {
    $nLoadTotal  += $cf->getResultField("P(${i})_ExeEngine_iLALU_LD","n");
    $nStoreTotal += $cf->getResultField("P(${i})_ExeEngine_iSALU_ST","n");

    $nInstTotal += getProcnInst($i);
  }

  # End Global Stats
  my $secs    = $cf->getResultField("OSSim","msecs");
  my $secs_gpu    = $cf->getResultField("OSSim","msecs_gpu");
  $secs_gpu = 1 if ($secs_gpu == 0);
  $secs_gpu = $secs_gpu/1000;
  my $nSampler    = $cf->getResultField("OSSim","nSampler");

  print "********************************************************************************************************\n";
  for(my $s=0; $s<$nSampler;$s++){ 

    my $sampler_totalclockticks = 0;
    my $ptr = @Samplers[$s];
    for(my $size=0;$size<scalar (@$ptr) ;$size++) {
      my $cpu_id = @Samplers[$s]->[$size];
      $sampler_totalclockticks  += $cf->getResultField("P(${cpu_id})","clockTicks");
    }

    if ($sampler_totalclockticks > 0){

      printf ("Sampler %d (Procs ",$s);

      for(my $size=0;$size<scalar (@$ptr) ;$size++) {
        my $cpu_id = @Samplers[$s]->[$size];
        printf(" %d",$cpu_id); 
      }
      printf (")");

      if (@SamplerType[$s] == 1 ){
        ptxStats();
      }
      printf ("\n");

      print "         Rabbit\tWarmup\tDetail\tTiming \tTotal  KIPS \n";

      $secs = 1 if( $secs == 0 );

      my $inst_t = 0;
      my $usec_t = 1;
      printf "  KIPS   ";
      foreach my $mode ("Rabbit", "Warmup", "Detail", "Timing") {
        my $inst = $cf->getResultField("S(${s})","${mode}Inst");
        my $usec = $cf->getResultField("S(${s})","${mode}Time");
        $inst_t = $inst_t + $inst;
        $usec_t = $usec_t + $usec;

        if ($usec == 0) {
          printf "  N/A\t";
        }else{
          printf "%5.0f\t",($inst/($usec/1000));
        }
      }
      printf "%5.0f\t",($inst_t/($usec_t/1000));

      
      foreach my $mode ("Rabbit", "Warmup", "Detail", "Timing") {
        my $usec = $cf->getResultField("S(${s})","${mode}Time");

        printf " %4.1f%%\t",(100*$usec/$usec_t);
      }

      if (@SamplerType[$s] == 0){
        #Sampler 0 is CPU
        printf "\t: Sim Time (s) %6.3f Exe ",$secs - $secs_gpu; #CHECKME
      }else{
        #Sampler 1 is GPU
        printf "\t: Sim Time (s) %6.3f Exe ",$secs_gpu;
      }

      my $clockTicks = 0;

      if ($nCPUs > 1){
        my $ptr = @Samplers[$s];
        for(my $size=0;$size<scalar (@$ptr) ;$size++) {
          my $cpu_id = @Samplers[$s]->[$size];
          if ($clockTicks < $cf->getResultField("P(${cpu_id})","clockTicks")) {
            $clockTicks = $cf->getResultField("P(${cpu_id})","clockTicks"); 
          }
        }
      } else {
        $clockTicks = $cf->getResultField("P(0)","clockTicks");
      }

      printf " %6.3f ms Sim",(1e-3/$freq)*$clockTicks;
      print " (${freq}MHz)\n";
      printf "  Inst  ";

      foreach my $mode ("Rabbit", "Warmup", "Detail", "Timing") {
        my $tmp= $cf->getResultField("S(${s})","${mode}Inst");
        printf " %4.1f%%\t",(100*$tmp/($inst_t+1));
      }

      my $progressedTime = $cf->getResultField("progressedTime", "max"); # ns metric
      printf "        : Approx Total Time %6.3f ms Sim",$progressedTime/1e6;
      print " (${freq}MHz)";

      print "\n********************************************************************************************************\n";
    }
  }

}

sub instStats {
  printf("-----------------------------------------------------------------------------------------------------------------------------------------\n");
  my $file = shift;

  print "Proc :  rawInst  :   nCommit   :   nInst   :  AALU   :  BALU   :  CALU   :  LALU   :  SALU   :  LD Fwd :    Replay    : Worst Unit  (clk)\n";

  for(my $i=0;$i<$nCPUs;$i++) {
    next unless( $cf->getResultField("P(${i})","clockTicks")>0 );
    printf " %3d :",$i;

    my $iAALU = $cf->getResultField("P(${i})_ExeEngine_iRALU","n")
      + $cf->getResultField("P(${i})_ExeEngine_iAALU","n");

    my $iBALU  = $cf->getResultField("P(${i})_ExeEngine_iBALU_RBRANCH","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_LBRANCH","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_RJUMP","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_LJUMP","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_RCALL","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_LCALL","n")
      + $cf->getResultField("P(${i})_ExeEngine_iBALU_RET","n");

    my $iLALU = $cf->getResultField("P(${i})_ExeEngine_iLALU_LD","n");

    my $iSALU = $cf->getResultField("P(${i})_ExeEngine_iSALU_ST","n")
      + $cf->getResultField("P(${i})_ExeEngine_iSALU_LL","n")
      + $cf->getResultField("P(${i})_ExeEngine_iSALU_SC","n")
      + $cf->getResultField("P(${i})_ExeEngine_iSALU_ADDR","n");

    my $iCALU = $cf->getResultField("P(${i})_ExeEngine_iCALU_FPMULT","n")
      + $cf->getResultField("P(${i})_ExeEngine_iCALU_FPDIV","n")
      + $cf->getResultField("P(${i})_ExeEngine_iCALU_FPALU","n")
      + $cf->getResultField("P(${i})_ExeEngine_iCALU_MULT","n")
      + $cf->getResultField("P(${i})_ExeEngine_iCALU_DIV","n");

    my $nInst = $iAALU + $iBALU + $iCALU + $iSALU + $iLALU;
    my $nFor   = $cf->getResultField("LSQ(${i})","stldForwarding");

    my $rawInst = $cf->getResultField("Reader(${i})","rawInst");

    $nInst = 1 if ($nInst == 0);

    printf "%10.0f : " ,$rawInst;
    printf "%10.0f  :" ,getProcnInst($i);
    printf "%10.0f :" ,$nInst;
    printf "  %5.2f%% :",100*$iAALU/$nInst;
    printf "  %5.2f%% :",100*$iBALU/$nInst;
    printf "  %5.2f%% :",100*$iCALU/$nInst;
    printf "  %5.2f%% :",100*$iLALU/$nInst;
    printf "  %5.2f%% :",100*$iSALU/$nInst;

    printf " %5.2f%%  :",100*$nFor/($iLALU+1);

    my $cpuType    = $cf->getConfigEntry(key=>"cpusimu",index=>$i);
    my $worstUnit;
    my $worstValue = 0;
    for (my $j=0; ; $j++) {
      my $clusterType = $cf->getConfigEntry(key=>"cluster", section=>$cpuType, index=>$j);
      last unless (defined $clusterType);


      foreach my $unitID ("RALU", "AALU"
          , "BALU_BRANCH", "BALU_JUMP", "BALU_CALL", "BALU_RET"
          , "SALU_ST", "SALU_LL", "SALU_SC", "SALU_ADDR"
          , "CALU_FPMULT", "CALU_FPDIV", "CALU_FPALU" , "CALU_MULT", "CALU_DIV") {

        my $tmp     = $cf->getConfigEntry(key=>"i${unitID}Unit", section=>$clusterType);
        next unless (defined $tmp);

        my $val     = $cf->getResultField("${tmp}(${i})_occ","v");

        if ($val > $worstValue ) {
          $worstValue = $val;
          $worstUnit  = $tmp;
        }
      }
    }

    my $nReplayInst_n = $cf->getResultField("P(${i})_nReplayInst","n"); 
    if ($nReplayInst_n==0) {
      $nReplayInst_n += $cf->getResultField("P(${i})_specld","stldViolations"); 
    }
    if ($nReplayInst_n) {
      printf "  %5.0f i/rep",getProcnInst($i)/$nReplayInst_n;
    }else{
      printf "      N/A    ";
    }

    printf " : ${worstUnit}  %4.2f ",$worstValue;

    print "\n";
  }
}

sub branchStats {
  my $file = shift;

  print "Proc : Avg.Time :  BPType           :  Total  :         RAS        :  BPred  :         BTB         :  BTAC";
  my $preType = $cf->getConfigEntry(key=>"preType");
  if( $preType > 0 ) {
    print "         " . $preType;
  }
  print "\n";

  for(my $i=0;$i<$nCPUs;$i++) {
    next unless( $cf->getResultField("P(${i})","clockTicks")>0 );

    my $cpuType    = $cf->getConfigEntry(key=>"cpusimu",index=>$i);

    my $branchSect  = $cf->getConfigEntry(key=>"bpred", section=>$cpuType);
    my $type        = $cf->getConfigEntry(key=>"type" , section=>$branchSect);
    my $smtContexts  = $cf->getConfigEntry(key=>"smtContexts",section=>$cpuType);
    $smtContexts++ if( $smtContexts == 0 );

    printf " %3d : ",$i;

################
    my $nBranches = 0;
    my $nMiss     = 0;
    my $avgBranchTime=0;
    for(my $j=0;$j<$smtContexts;$j++) {
      my $id = $i*$smtContexts+$j;

      $nBranches += $cf->getResultField("P(${id})_BPred","nBranches");
      $nMiss     += $cf->getResultField("P(${id})_BPred","nMiss");
      $avgBranchTime += $cf->getResultField("P(${id})_FetchEngine_avgBranchTime","v");
    }
    $avgBranchTime /= $smtContexts;
    if( $nBranches == 0 ) {
      $nBranches = 1;
    }

    printf "%8.3f :  ",$avgBranchTime;

    printf "%-16s :",$type;

    printf "%7.2f%% :",100*($nBranches-$nMiss)/($nBranches);

################
    my $rasHit  = 0;
    my $rasMiss = 0;
    for(my $j=0;$j<$smtContexts;$j++) {
      my $id = $i*$smtContexts+$j;
      $rasHit  += $cf->getResultField("P(${id})_BPred_RAS","nHit");
      $rasMiss += $cf->getResultField("P(${id})_BPred_RAS","nMiss");
    }

    my $rasRatio = ($rasMiss+$rasHit) <= 0 ? 0 : ($rasHit/($rasMiss+$rasHit));

    printf " (%6.2f%% of %4.2f%%) :",100*$rasRatio ,100*($rasHit+$rasMiss)/$nBranches;

################
    my $predHit  = $cf->getResultField("P(${i})_BPred_${type}","nHit");
    my $predMiss = $cf->getResultField("P(${i})_BPred_${type}","nMiss");

    my $predRatio = ($predMiss+$predHit) <= 0 ? 0 : ($predHit/($predMiss+$predHit));

    printf " %6.2f%% :",100*$predRatio;

################
    my $btbHit  = $cf->getResultField("P(${i})_BPred_BTB","nHit");
    my $btbMiss = $cf->getResultField("P(${i})_BPred_BTB","nMiss");

    my $btbRatio = ($btbMiss+$btbHit) <= 0 ? 0 : ($btbHit/($btbMiss+$btbHit));

    printf " (%6.2f%% of %4.2f%%) :",100*$btbRatio ,100*($btbHit+$btbMiss)/$nBranches;

    my $nBTAC = 0;
    for(my $j=0;$j<$smtContexts;$j++) {
      my $id = $i*$smtContexts+$j;

      $nBTAC += $cf->getResultField("P(${id})_FetchEngine","nBTAC");
    }
    printf "%6.2f%% ",100*$nBTAC/$nBranches;
################

    my $rapHit  = $cf->getResultField("_BPred(${i})_Rap","nHit")
      + $cf->getResultField("BPred(${i})_CRap","nHit");
    if( $rapHit ) {
      my $rapMiss = $cf->getResultField("BPred(${i})_Rap","nMiss")
        + $cf->getResultField("BPred(${i})_CRap","nMiss");

      my $rapRatio = ($rapMiss+$rapHit) <= 0 ? 0 : ($rapHit/($rapMiss+$rapHit));

      printf "(%6.2f%% of %6.2f%%) ",100*$rapRatio ,100*($rapHit+$rapMiss)/$nBranches;
    }

    print "\n";
  }
}

sub thermStats_table {

  my $file = shift;

  my $totCycles = 0;
  my $nCycles=0;
  for(my $i=0;$i<$nCPUs;$i++) {
    my $val = $cf->getResultField("P(${i})","clockTicks");
    $totCycles += $val;
    $nCycles = $val if ($nCycles < $val);
    }

  return unless $cf->getResultField("chipMaxT","max");

  printf "################################################################################\n";
  print "Thermal Metrics:\n";

  my $scaleFactor = 1e-10;
  my $scaleFactorSM = 1e-30;
  my $nFLPblks  = 19; 
  my $total_area = 0;
  my $reli_mttf = 0;
  my $chippower = 0;
  my $chipleak = 0;

  my $frequency = 0;
  my $technology = 0;
  my $total_power = 0;
  
  my $em;
  my $sm;
  my $tc;
  my $tddb;
  my $nbti;

  $technology = $cf->getConfigEntry(key=>"technology");
  $frequency  = $cf->getConfigEntry(key=>"frequency", section=>$technology);

  my $simulatedTime  = ($cf->getResultField("simulatedTime","max"));
  for(my $i=0;$i<$nFLPblks;$i++) {
    my $thermal    = $cf->getConfigEntry(key=>"thermal");
    my $floorplan  = $cf->getConfigEntry(key=>"floorplan",section=>$thermal);
    my $blk  = $cf->getConfigEntry(key=>"blockDescr",section=>$floorplan, index=>$i);
    my @txt  = split(/ /, $blk);
    my $area = $txt[1] * $txt[2]; 
    $total_area += $area;

    $em  = $cf->getResultField("flpBlock[${i}]_EM_fit","v")*$area;
    $sm  = $cf->getResultField("flpBlock[${i}]_SM_fit","v")*$area;
    $tc  = $cf->getResultField("flpBlock[${i}]_TC_fit","v")*$area;
    $tddb= $cf->getResultField("flpBlock[${i}]_TDDB_fit","v")*$area;
    $nbti= $cf->getResultField("flpBlock[${i}]_NBTI_fit","v")*$area;
    my $em_n  = $cf->getResultField("flpBlock[${i}]_EM_fit","n");
			my $sm_n  = $cf->getResultField("flpBlock[${i}]_SM_fit","n");
    my $tc_n  = $cf->getResultField("flpBlock[${i}]_TC_fit","n");
    my $tddb_n= $cf->getResultField("flpBlock[${i}]_TDDB_fit","n");
    my $nbti_n= $cf->getResultField("flpBlock[${i}]_NBTI_fit","n");
    $em  = $em * $scaleFactor ;
    $sm  = $sm * $scaleFactorSM ;
    $tc  = $tc *  $tc_n;
    $tddb  = $tddb * $scaleFactor ;
    $nbti  = $nbti * $scaleFactor ;
    $reli_mttf += ($total_area*$simulatedTime)/($em + $sm + $tc + $tddb + $nbti+1);
  } 
  my $maxT= ($cf->getResultField("chipMaxT","max"));
  my $gradT= ($cf->getResultField("chipGradT","max"));

  $chippower =$cf->getResultField("ChipPower","v");
  $chipleak = $cf->getResultField("ChipLeak","v");
  my $pwrmodel   = $cf->getConfigEntry(key=>"pwrmodel");
  my $throttleCycleRatio  = $cf->getConfigEntry(key=>"throttleCycleRatio", section=>$pwrmodel);
  my $updateInterval  = $cf->getConfigEntry(key=>"updateInterval", section=>$pwrmodel);
  my $thermalThrottle   = $cf->getConfigEntry(key=>"thermalThrottle", section=>$pwrmodel);

  my $sescThermTime  = ($cf->getResultField("sescThermTime","max"));
  my $throttlingCycles  = $cf->getResultField("ThrottlingCycles", "max");
  my $ThrottlingPerc = 100.0 * $throttlingCycles*$updateInterval*$throttleCycleRatio/($frequency*$simulatedTime);

  # GET IPC
  my $nInst = 0;        
  my $nuInst = 0;        
  my $clockTicks;

  for(my $i=0;$i<$nCPUs;$i++) {
      $clockTicks = $cf->getResultField("P(${i})","clockTicks");
      next unless( $clockTicks > 1 );
      $nInst = $cf->getResultField("S(${i})","TimingInst");
      $nuInst  = getProcnInst($i);
  }
  my $name = $file;
  $name =~ /.*sesc\_([^ ]*)......./;
  $name = $1;
  printf "#table10 name\t\t IPC\t maxT(K)\t gradT\t Reliability\tChipLeak(W)\tChipPower(W)\tEnergy(J)\ttIPC\t uIPC\n";
  my $ipc = 0;
  my $uipc = 0;
  if ($clockTicks != 0){
      $ipc = $nInst/($clockTicks);
      $uipc = $nuInst/($clockTicks);
  }
  # maximum temperature throughout the chip
  printf "table10:\t%s:\t%f:\t%f:\t", $name,$ipc, $maxT;
  # largest temperature difference across chip
  printf "%f:",$gradT;
  # reliablility metrics
  printf "%g:",$reli_mttf;
  printf "%g:",$chipleak;
  printf  "%g:",$chippower;
  printf  "%f:",($chippower*$simulatedTime);
  printf  "%f:",($nInst/($frequency*$simulatedTime));
  printf "%f\n",$uipc;
  
  # Thermal Throttling
  printf "#table11\t %15s\t\t Freq\t  ThrotCyleRatio\t ThrotThresh \t ThrotCycles\t \%Throttling\t sescThermTime\t SimulatedTime \n";
  printf "table11\t %15s\t\t %5e\t %9u\t %9u\t \t%9g\t \t%9.4g\t \t%9g\t \t%2.3g\n",
  $1, $frequency, $throttleCycleRatio,$thermalThrottle, $throttlingCycles, $ThrottlingPerc, $sescThermTime, $simulatedTime;
}

sub tradCPUStats {
  my $file = shift;

  print "Proc  IPC     uIPC";
  print "    Active    ";
  print "    Cycles    Busy   LDQ   STQ  IWin   ROB";
  print "  Regs    IO  maxBr  MisBr Br4Clk brDelay \n";

  my $inst_t = 0;
  my $cycle_t = 1;
  my $globalc_max = 0;
  my $nActiveCores = 0;
  my $gIPC = 0;

  my $tinst = 0;
  my $tcycle = 1;
  my $tcores = 0;

  for(my $i=0;$i<$nCPUs;$i++) {  

    my $sampler     = @Processors[$i]->[0]; 
    my $globalClock = 0;
    if ($sampler == 0){
      $globalClock = $cf->getResultField("S(0)","globalClock_Timing")+1;
    } else {
      $globalClock = $cf->getResultField("S(${sampler})","globalClock_Timing")+1;
    }

    if ($globalClock > $globalc_max) {
      $globalc_max = $globalClock;
    }

    my $clockTicks  = $cf->getResultField("P(${i})","clockTicks");

    $cycle_t += $clockTicks;

    next unless( $clockTicks > 1 );

    $gIPC += $cf->getResultField("P(${i})_uipc","v");
    $nActiveCores++;
    my $nInst       = getProcnInst($i);
    $inst_t += $nInst;
    my $timingInst  = $cf->getResultField("S(${i})","TimingInst");

    my $sampler     = @Processors[$i]->[0]; 
    if (@SamplerType[$sampler] == 0){
      $tcycle += $clockTicks;
      $tinst += $nInst;
      $tcores += 1;
    }

    my $cpuType     = $cf->getConfigEntry(key =>"cpusimu",index       =>$i);
    my $fetch       = $cf->getConfigEntry(key =>"fetchWidth",section  =>$cpuType);
    my $issue       = $fetch;

    my $smtContexts = $cf->getConfigEntry(key =>"smtContexts",section =>$cpuType);
    $smtContexts++ if( $smtContexts == 0 );

    my $rawInst = $cf->getResultField("Reader(${i})","rawInst");
    my $temp    = $cf->getConfigEntry(key=>"issueWidth",section=>$cpuType);
    $issue      = $temp if( $temp < $issue && $temp );

  ##########################
  # pid, IPC, uIPC, active, cycles

  printf " %3d  %05.2f  % 5.2f  ",$i,$timingInst/$clockTicks,$nInst/$clockTicks;
  printf " %- 12.2f",$clockTicks/$globalClock;
  printf " %- 9.0f  ",$clockTicks;

  ##########################
  # Substract BUSY time

    $nInst = 1 if( $nInst < 1 );

    my $idealInst = $issue*$clockTicks;
    $temp = 100*$nInst/($idealInst);

    printf " %04.1f ",$temp;

    my $remaining = 100; # 100% of the time
    $remaining-=$temp;

    ##########################
    # Window %

    my $nOutsLoads = $cf->getResultField("P(${i})_ExeEngine","nOutsLoadsStall");
    $temp = 100*$nOutsLoads/$idealInst;
    printf " %4.1f ",$temp;
    $remaining -= $temp;

    my $nOutsStores = $cf->getResultField("P(${i})_ExeEngine","nOutsStoresStall");
    $temp = 100*$nOutsStores/$idealInst;
    printf " %4.1f ",$temp;
    $remaining -= $temp;

    my $nSmallWin = $cf->getResultField("P(${i})_ExeEngine","nSmallWinStall");
    $temp = 100*$nSmallWin/$idealInst;
    printf " %4.1f ",$temp;
    $remaining -= $temp;

    my $nSmallROB = $cf->getResultField("P(${i})_ExeEngine","nSmallROBStall");
    $temp = 100*$nSmallROB/$idealInst;
    printf " %4.1f ",$temp;
    $remaining -= $temp;

    my $nSmallREG = $cf->getResultField("P(${i})_ExeEngine","nSmallREGStall");
    $temp = 100*$nSmallREG/$idealInst;
    printf " %4.1f ",$temp;
    $remaining -= $temp;

    my $nSyscall = $cf->getResultField("P(${i})_ExeEngine","nSyscallStall");
    $temp = 100*$nSyscall/$idealInst;
    printf " %4.1f ",$temp;
    $remaining -= $temp;

    my $nReplays = $cf->getResultField("P(${i})_ExeEngine","ReplaysStall");
    my $nReplayInst = $cf->getResultField("P(${i})_nReplayInst","v"); 
    my $nReplayInst_n = $cf->getResultField("P(${i})_nReplayInst","n");
    $remaining -= $temp;

    my $nOutsBranches = $cf->getResultField("P(${i})_ExeEngine","nOutsBranches");
    $temp = 100*$nOutsBranches/$idealInst;
    printf " %4.1f ",$temp;
    $remaining -= $temp;

    ##########################
    # The rest is the control %

    my $nDelayInst1 = 0;
    my $nDelayInst2 = 0;
    my $nDelayInst3 = 0;
    for(my $j=0;$j<$smtContexts;$j++) {
      my $id = $i*$smtContexts+$j;

      $nDelayInst1  += $cf->getResultField("P(${i})_FetchEngine","nDelayInst1");
      $nDelayInst2  += $cf->getResultField("P(${i})_FetchEngine","nDelayInst2");
      $nDelayInst3  += $cf->getResultField("P(${i})_FetchEngine","nDelayInst3");
    }

    $temp = 100*$nDelayInst1/$idealInst;
    $remaining -= $temp;
    printf " %5.1f ",$temp;

    $temp = 100*$nDelayInst2/$idealInst;
    $remaining -= $temp;
    printf " %5.1f ",$temp;

    $temp = 100*$nDelayInst3/$idealInst;
    $remaining -= $temp;
    printf " %5.1f ",$temp;

    print "\n";
  }
  my $progressedTime = $cf->getResultField("progressedTime", "max");
}




sub cpupowerStats {

  my $file = shift;

  if ($op_enPower eq "true") {
    printf "********************************************************************************************************\n";
    print "CPU Power Metrics: (Dynamic Power,Leakage Power) \n";
    printf( "%-4s %- 13s %- 13s %- 13s %- 13s %- 13s %- 13s %- 13s %- 13s%- 18s\n",
      "Proc",
      "|  RF (mW)",
      "|  ROB (mW)",
      "|  fetch (mW)",
      "|  EXE (mW)", 
      "|  RNU (mW)", 
      "|  LSU (mW)", 
      "|  Total (W)");
      #  "|  Chip Total (W)");

  printf("--------------------------------------------------------------------------------------------------------\n");

    for(my $i=0;$i<$nCPUs;$i++) {

      next unless( $cf->getResultField("P(${i})","clockTicks") );
      
      my $sampler     = @Processors[$i]->[0]; 
      if (@SamplerType[$sampler] == 0){

        printf "%- 4d ",$i;

        my $mypow = $cf->getResultField("pwrDynP(${i})_RF","v");
        my $mypowl = $cf->getResultField("pwrLkgP(${i})_RF","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_chippower += $mypow;
        $op_chippower_L += $mypowl;


        $mypow = $cf->getResultField("pwrDynP(${i})_ROB","v");
        $mypowl = $cf->getResultField("pwrLkgP(${i})_ROB","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_chippower += $mypow;
        $op_chippower_L += $mypowl;


        $mypow = $cf->getResultField("pwrDynP(${i})_fetch","v");
        $mypowl = $cf->getResultField("pwrLkgP(${i})_fetch","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_chippower += $mypow;
        $op_chippower_L += $mypowl;


        $mypow = $cf->getResultField("pwrDynP(${i})_EXE","v");
        $mypowl = $cf->getResultField("pwrLkgP(${i})_EXE","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_chippower += $mypow;
        $op_chippower_L += $mypowl;


        $mypow = $cf->getResultField("pwrDynP(${i})_RNU","v");
        $mypowl = $cf->getResultField("pwrLkgP(${i})_RNU","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_chippower += $mypow;
        $op_chippower_L += $mypowl;


        $mypow = $cf->getResultField("pwrDynP(${i})_LSU","v");
        $mypowl = $cf->getResultField("pwrLkgP(${i})_LSU","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_chippower += $mypow;
        $op_chippower_L += $mypowl;

        printf "| %- 6.2f,%- 3.2f " ,$op_chippower/1000,$op_chippower_L/1000;
#      printf " | %- 10.2f " ,($op_chippower + $op_chippower_L)/1000;
        print "\n";
        $op_chippower = 0;
        $op_chippower_L = 0;
      }
    }
  }
}


sub gpupowerStats {

  my $file = shift;
  my $op_SMpower = 0;
  my $op_SMpower_L = 0;
  my $op_GPUpower = 0;
  my $op_GPUpower_L = 0;

  if ($op_enPower eq "true") {
    printf "********************************************************************************************************\n";
    print "GPU Power Metrics: (Dynamic Power,Leakage Power) \n";
    printf( "%-4s %- 13s %- 13s %- 13s %- 13s %- 13s %- 13s %- 13s %- 13s%- 18s\n",
      "Proc",
      "|  RF (mW)",
      "|  Lanes (mW)",
      "|  DL1G (mW)",
      "|  IL1G (mW)",
      "|  Total (W)");
      #  "|  Chip Total (W)");

  printf("--------------------------------------------------------------------------------------------------------\n");

    for(my $cpu=0;$cpu<$nCPUs;$cpu++) {
      next unless( $cf->getResultField("P(${cpu})","clockTicks") );

      my $sampler     = @Processors[$cpu]->[0]; 
      if (@SamplerType[$sampler] == 1){
        my $i = $cpu-1;  #BUGGY BUGGY BUGGY
        printf "%- 4d ",$cpu;

        my $mypow = $cf->getResultField("pwrDynRFG(${i})","v");
        my $mypowl = $cf->getResultField("pwrLkgRFG(${i})","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_SMpower += $mypow;
        $op_SMpower_L += $mypowl;


        $mypow = $cf->getResultField("pwrDynlanesG(${i})","v");
        $mypowl = $cf->getResultField("pwrLkglanesG(${i})","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_SMpower += $mypow;
        $op_SMpower_L += $mypowl;


        $mypow = $cf->getResultField("pwrDynDL1G(${i})","v");
        $mypowl = $cf->getResultField("pwrLkgDL1G(${i})","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_SMpower += $mypow;
        $op_SMpower_L += $mypowl;


        $mypow = $cf->getResultField("pwrDynIL1G(${i})","v");
        $mypowl = $cf->getResultField("pwrLkgIL1G(${i})","v");
        printf "| %- 6.0f,%- 4.0f " ,$mypow,$mypowl;
        $op_SMpower += $mypow;
        $op_SMpower_L += $mypowl;

        printf "| %- 6.2f,%- 3.2f " ,$op_SMpower/1000,$op_SMpower_L/1000;
        $op_GPUpower  += $op_SMpower;
        $op_GPUpower_L  += $op_SMpower_L;
#      printf " | %- 10.2f " ,($op_SMpower + $op_SMpower_L)/1000;
        print "\n";
        $op_SMpower = 0;
        $op_SMpower_L = 0;
      }
    }
    printf("--------------------------------------------------------------------------------------------------------\n");
    if ($op_GPUpower > 0){

        my $mypow = $cf->getResultField("pwrDynL2G(0)","v");
        my $mypowl = $cf->getResultField("pwrLkgL2G(0)","v");
        $op_GPUpower += $mypow;
        $op_GPUpower_L += $mypowl;
        printf "L2G Power = %- 6.0f (Dyn) ,%- 4.0f (Lkg)\n" ,$mypow,$mypowl;


        $mypow = $cf->getResultField("pwrDynL3(0)","v");
        $mypowl = $cf->getResultField("pwrLkgL3(0)","v");
        $op_GPUpower += $mypow;
        $op_GPUpower_L += $mypowl;
        printf "L3G Power = %- 6.0f (Dyn) ,%- 4.0f (Lkg)\n" ,$mypow,$mypowl;


      printf "Total GPU Power =  %- 6.2f (Dyn) %- 3.2f (Lkg) \n" ,$op_GPUpower/1000,$op_GPUpower_L/1000;
      printf("--------------------------------------------------------------------------------------------------------\n");
    }
  }
}

sub powerStats {
  my $file = shift;
  cpupowerStats($file);
  #gpupowerStats($file);
}

sub thermStats{

  my $file = shift;

  my $powersec = $cf->getConfigEntry(key=>"pwrmodel");

  if ($op_enTherm eq "true") {


    my $totCycles = 0;
    my $nCycles=0;
    for(my $i=0;$i<$nCPUs;$i++) {
      my $val = $cf->getResultField("P(${i})","clockTicks");
      $totCycles += $val;
      $nCycles = $val if ($nCycles < $val);
    }

    return unless $cf->getResultField("chipMaxT","max");

    printf "\n********************************************************************************************************\n";
    print "Thermal Metrics:\n";

    my $scaleFactor = 1e-10;
    my $scaleFactorSM = 1e-30;
    my $nFLPblks  = 19; 
    my $total_area = 0;
    my $reli_mttf = 0;
    my $chippower = 0;
    my $chipleak = 0;

    my $frequency = 0;
    my $technology = 0;
    my $total_power = 0;

    my $em;
    my $sm;
    my $tc;
    my $tddb;
    my $nbti;

    $technology = $cf->getConfigEntry(key=>"technology");
    $frequency  = $cf->getConfigEntry(key=>"frequency", section=>$technology);

    my $simulatedTime  = ($cf->getResultField("simulatedTime","max"));
    for(my $i=0;$i<$nFLPblks;$i++) {
      my $thermal    = $cf->getConfigEntry(key=>"thermal");
      my $floorplan  = $cf->getConfigEntry(key=>"floorplan",section=>$thermal);
      my $blk  = $cf->getConfigEntry(key=>"blockDescr",section=>$floorplan, index=>$i);
      my @txt  = split(/ /, $blk);
      my $area = $txt[1] * $txt[2]; 
      $total_area += $area;

      $em  = $cf->getResultField("flpBlock[${i}]_EM_fit","v")*$area;
      $sm  = $cf->getResultField("flpBlock[${i}]_SM_fit","v")*$area;
      $tc  = $cf->getResultField("flpBlock[${i}]_TC_fit","v")*$area;
      $tddb= $cf->getResultField("flpBlock[${i}]_TDDB_fit","v")*$area;
      $nbti= $cf->getResultField("flpBlock[${i}]_NBTI_fit","v")*$area;
      my $em_n  = $cf->getResultField("flpBlock[${i}]_EM_fit","n");
      my $sm_n  = $cf->getResultField("flpBlock[${i}]_SM_fit","n");
      my $tc_n  = $cf->getResultField("flpBlock[${i}]_TC_fit","n");
      my $tddb_n= $cf->getResultField("flpBlock[${i}]_TDDB_fit","n");
      my $nbti_n= $cf->getResultField("flpBlock[${i}]_NBTI_fit","n");
      $em  = $em * $scaleFactor ;
      $sm  = $sm * $scaleFactorSM ;
      $tc  = $tc *  $tc_n;
      $tddb  = $tddb * $scaleFactor ;
      $nbti  = $nbti * $scaleFactor ;
      $reli_mttf += ($total_area*$simulatedTime)/($em + $sm + $tc + $tddb + $nbti+1);
    } 
    my $maxT= ($cf->getResultField("chipMaxT","max"));
    my $gradT= ($cf->getResultField("chipGradT","max"));

    $chippower =$cf->getResultField("ChipPower","v");
    $chipleak = $cf->getResultField("ChipLeak","v");
    my $pwrmodel   = $cf->getConfigEntry(key=>"pwrmodel");
    my $throttleCycleRatio  = $cf->getConfigEntry(key=>"throttleCycleRatio", section=>$pwrmodel);
    my $updateInterval  = $cf->getConfigEntry(key=>"updateInterval", section=>$pwrmodel);
    my $thermalThrottle   = $cf->getConfigEntry(key=>"thermalThrottle", section=>$pwrmodel);

    my $sescThermTime  = ($cf->getResultField("sescThermTime","max"));
    my $throttlingCycles  = $cf->getResultField("ThrottlingCycles", "max");
    my $ThrottlingPerc = 100.0 * $throttlingCycles*$updateInterval*$throttleCycleRatio/($frequency*$simulatedTime);

    my $name = $file;
    $name =~ /.*sesc\_([^ ]*)......./;
    $name = $1;
    printf "maxT(K)      gradT      Reliability     ChipLeak(W)    ChipPower(W)     Energy(J)\n";
    
    #maximum temperature throughout the chip
    printf "%.3f", $maxT;
    #largest temperature difference across chip
    printf "    %.3f",$gradT;
    #reliablility metrics
    printf "        %.3g",$reli_mttf;
    printf "        %.3g",$chipleak;
    printf "          %.3g",$chippower;
    printf "            %.3f",($chippower*$simulatedTime);

    print "\n\n";

    #Thermal Throttling
    printf "ThrotCyleRatio      ThrotThresh ThrotCycles\t    Throttling\t     sescThermTime\t SimulatedTime \n";
    printf "%9u    %9u   %9.3g\t \t%9.4g\t \t%9g\t\t%2.3g\n",
    $throttleCycleRatio,$thermalThrottle, $throttlingCycles, $ThrottlingPerc, $sescThermTime, $simulatedTime;
  }
}
