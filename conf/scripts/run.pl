#!/usr/bin/env perl

use strict;

use Getopt::Long;
use Time::localtime;
use Env;
use File::Spec;

my $BHOME;

my $op_test=0;
my $op_dump=0;
my $op_sesc="$ENV{'SESCBUILDDIR'}/sesc";
my $op_key;
my $op_key2;
my $op_ext;
my $op_load=1;
my $op_mload=4;
my $op_clean;
my $op_bhome=$ENV{'BENCHDIR'};
my $op_prof;
my $op_vprof;
my $op_profsec;
my $op_c="$ENV{'SESCBUILDDIR'}/sesc.conf";
my $op_fast;
my $op_data="ref";
my $op_numprocs=1;
my $op_rabbit;
my $op_full;
my $op_saveoutput;
my $op_ninst;
my $op_spoint;
my $op_spointsize=100000000; # 100M by default
my $op_native;
my $op_bindir="$ENV{'BENCHDIR'}/bin";
my $op_yes;
my $op_qsub;
my $op_condor;
my $op_condorstd;
my $op_trace;
my $op_email;
my $op_help=0;

my $result = GetOptions("test",\$op_test,
                        "dump",\$op_dump,
                        "sesc=s",\$op_sesc,
                        "key=s",\$op_key,
                        "key2=s",\$op_key2,
                        "ext=s",\$op_ext,
                        "load=i",\$op_load,
                        "mload=i",\$op_mload,
                        "bhome=s",\$op_bhome,
                        "email=s",\$op_email,
                        "fast",\$op_fast,
                        "vprof",\$op_vprof,
                        "prof=i",\$op_prof,
                        "profsec=s",\$op_profsec,
                        "clean",\$op_clean,
                        "c=s",\$op_c,
                        "data=s",\$op_data,
                        "procs=i",\$op_numprocs,
                        "rabbit", \$op_rabbit,
                        "full", \$op_full,
			"saveoutput",\$op_saveoutput,
			"bindir=s",\$op_bindir,
			"native",\$op_native,
			"ninst=i",\$op_ninst,
                        "spoint=i",\$op_spoint,
                        "spointsize=i",\$op_spointsize,
			"yes",\$op_yes,
			"qsub",\$op_qsub,
			"condor",\$op_condor,
			"condorstd",\$op_condorstd,
			"trace",\$op_trace,
			"help",\$op_help
		       );


my $dataset;
my $jobfp;
my $threadsRunning=0;

sub waitUntilLoad {
  my $load= shift;

  my $loadavg = 0;

  sleep 6*rand();

  open(FH, "ps axu|") or die ('Failed to open file');
  while (<FH>) {
      chop();
      my $line = $_;
      next if ( $line =~ /COMMAND|axu|grep/);
      next unless ($line =~ /\ R/);

      $loadavg++;
  }
  close(FH);

  if ($loadavg >= $op_mload) {
      # There may be multiple run.pl running. It is best if we wait a bit until
      # uptime gets more stable
      print "Too many running procs (${loadavg} waiting...\n";
      sleep 64*rand()+23;
  }

  while (1) {
    open(FH, "uptime|") or die ('Failed to open file');
    while (<FH>) {
      chop();
      # First occurence of loadavg
      if (/(\d+\.\d+)/ ) {
        $loadavg = $1;
        last;
      }
    }
    close(FH);
    last if ($loadavg <= $load);
    print "Load too high (${loadavg} waiting...\n";
    sleep 128*rand();
  }
}

sub getExec {
  my $benchName = shift;

  return "${op_bindir}/${benchName}${op_ext}";
}

sub getMarks {
  my $fastMarks = shift;
  my $normalMarks = shift;

  return $normalMarks if ($op_vprof);

  return "" if (defined($op_rabbit) and !defined($op_prof));

  my $marks = $normalMarks;

  return "" if (defined($op_full));

  if (defined($op_spoint)) {
    $marks = "-w" . ($op_spoint*$op_spointsize) . " -y$op_spointsize";
    return $marks;
  }

  $marks = $fastMarks if ($op_fast or defined($op_prof));

  if ($op_native ) {
      if ( $marks =~ /-1([[:digit:]]+) -2([[:digit:]]+)/ ) {
          $ENV{"SESC_1"} = $1;
          $ENV{"SESC_2"} = $2;
      }
  }

  return $marks;
}

sub runItLowLevel {
    my $sprg = shift;
    my $aprg = shift;

    # Refresh the automount. Linux AS 2.0 automount problem
    if( $op_dump ) {
        print "[${sprg} ${aprg}]\n";
    }elsif ($op_native) {
        system($aprg);
    }else{
        # Run the app
        system("${sprg} ${aprg}");
    }
}

sub newQSub {
  my $sprg  = shift;
  my $aprg  = shift;
  my $bench = shift;

  my $fileid;
  if ( defined($op_key) ) {
    if (defined($op_key2) ) {
      $fileid = "${op_key}_${op_key2}";
    } else {
      $fileid = "${op_key}";
    }
  } else {
    if (defined($op_key2) ) {
      $fileid = "${op_ext}_${op_key2}";
    } else {
      $fileid = "${op_ext}";
    }
  }

  open(JOB, ">run_qsub_${bench}_${fileid}.sh");

  print JOB "#!/bin/sh\n";
  print JOB "#\$ -S /bin/sh\n";
  print JOB "#\$ -m es\n";
  print JOB "#\$ -M ${op_email}\n";
  print JOB "#\$ -cwd\n";
  print JOB "#\$ -o run_qsub_${bench}_${fileid}.log\n";
  print JOB "#\$ -e run_qsub_${bench}_${fileid}.err\n";
  print JOB "#\$ -N \"${bench} ${fileid}\"\n";
  print JOB "uname -a\n";

  foreach my $key (sort keys(%ENV)) {
    if ($key =~ /SESC/) {
      print JOB "export $key=$ENV{$key}\n";
    }
  }

  print JOB "\n${sprg} ${aprg} | gzip -c >trace_${bench}_${fileid}.gz\n";

  print JOB "exit 0";

  close(JOB);
}

sub runIt {
  my $sesc_param = shift;
  my $app_param  = shift;
  my $inp        = shift;
  my $outp       = shift;
  my $bench      = shift;

  if($op_condor) { 
      newJob("${sesc_param} ${app_param}", $inp, $outp);
      return;
  }

  if($op_saveoutput) {
      $outp = "&> ${outp}.out"; 
  } else {
      $outp = "";
  }

  $inp  = "\< ${inp}" if($inp);
  my $sprg = "${op_sesc} ${sesc_param} ";
  my $aprg = "${app_param} ${inp} ${outp}";

  print RUNLOG "Launching: ";
  print RUNLOG "(Testing) " if( $op_test);
  print RUNLOG $sprg . $aprg . "\n";

  if($op_qsub && !$op_test) {
      newQSub($sprg, $aprg, $bench);
      return;
  }

  waitUntilLoad($op_mload);

  if( $threadsRunning >= $op_load ) {
    print RUNLOG "Maximum load reached. Waiting for someone to complete\n";
    wait();
    $threadsRunning--;
  }

  if( $op_load == 1 ) {
      runItLowLevel($sprg, $aprg);
  }elsif( fork() == 0 ) {
      # Child
      runItLowLevel($sprg, $aprg);
      exit(0);
  }else{
      $threadsRunning++;
  }
}

sub newJob {
  my $sparm = shift;
  my $inp = shift;
  my $outp = shift;
  
  printf $jobfp "Arguments    = $sparm\n";
  printf $jobfp "Input        = $inp\n";
  printf $jobfp "Log          = condorout/$outp.\$(Cluster).\$(Process).log\n";
  printf $jobfp "Output       = condorout/$outp.\$(Cluster).\$(Process).out\n";
  printf $jobfp "Error        = condorout/$outp.\$(Cluster).\$(Process).err\n";
  printf $jobfp "Queue\n\n";
}

sub runTrace {
  my %param = @_;

  die if( !defined($param{bench}) );

  my $sesc_parms = "";

  $sesc_parms .= " -x" . $param{xtra} if( defined($param{xtra}) );
  $sesc_parms .= " -f" . $param{xtra2} if( defined($param{xtra2}) );
  $sesc_parms .= " -t" if( $op_test );
  $sesc_parms .= " -c" . $op_c if( defined $op_c );
  $sesc_parms .= " -w1" if ($op_rabbit or $op_vprof);
  $sesc_parms .= " -r" . $op_prof if (defined $op_prof);
  $sesc_parms .= " -S" . $op_profsec if (defined $op_profsec);
  $sesc_parms .= " -y" . $op_ninst if(defined $op_ninst);

  my $baseoutput;

  my $vol;
  my $dir;
  my $traceName;

  ($vol,$dir,$traceName) = File::Spec->splitpath($param{bench});

  if ( defined($op_key) ) {
      if (defined($op_key2) ) {
	  $baseoutput = "${traceName}${op_ext}.${op_key}.${op_key2}";
      } else {
	  $baseoutput = "${traceName}${op_ext}.${op_key}";
      }
  } else {
      if (defined($op_key2) ) {
	  $baseoutput = "${traceName}${op_ext}.${op_key2}";
      } else {
	  $baseoutput = "${traceName}${op_ext}";
      }
  }
  
  runIt("${sesc_parms} ${param{bench}}", "", $baseoutput, $param{bench});
}

sub runBench {
  my %param = @_;

  die if( !defined($param{bench}) );

  my $sesc_parms = "";

  $sesc_parms .= " -x" . $param{xtra} if( defined($param{xtra}) );
  $sesc_parms .= " -f" . $param{xtra2} if( defined($param{xtra2}) );
  $sesc_parms .= " -t" if( $op_test );
  $sesc_parms .= " -c" . $op_c if( defined $op_c );
  $sesc_parms .= " -w1" if ($op_rabbit or $op_vprof);
  $sesc_parms .= " -r" . $op_prof if (defined $op_prof);
  $sesc_parms .= " -S" . $op_profsec if (defined $op_profsec);
  $sesc_parms .= " -y" . $op_ninst if(defined $op_ninst);

  my $baseoutput = "${param{bench}}${op_ext}";
  $baseoutput .= ".${op_key}"  if ( defined($op_key)  );
  $baseoutput .= ".${op_key2}" if ( defined($op_key2) );

  $baseoutput .= ".SP${op_spoint}" if ( defined($op_spoint) );

  my $executable = getExec($param{bench});

  ############################################################
  # CINT2000
  ############################################################
  if( $param{bench} eq 'gzip' ) {
    print "Running gzip\n";

    # Comments: Very homogeneous missrate across marks
    my $marks = getMarks("-120 -227", "-14 -2170");
    my $gzipinput;
    if ($op_data eq 'test') {
      $gzipinput = "input.compressed 2";
    } elsif ($op_data eq 'train') {
      $gzipinput = "input.combined 32";
    } else {
      $gzipinput = "input.source 60";
    }
    runIt("${sesc_parms} -h0xF000000 ${marks}","${executable} ${BHOME}/CINT2000/164.gzip/${dataset}/${gzipinput}", "", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'vpr' ) {
    print "Running vpr\n";

    my $marks = getMarks("-11 -22", "-11 -22");
    my $opt = "${BHOME}/CINT2000/175.vpr/${dataset}/net.in ${BHOME}/CINT2000/175.vpr/${dataset}/arch.in ";
    $opt .= "place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 ";
    $opt .= "-alpha_t 0.9412 -inner_num 2";
    runIt("${sesc_parms} -h0xc00000 ${marks}","${executable} ${opt}", "", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'gcc' ) {
    print "Running gcc\n";

    my $marks = getMarks("-14 -26", "-16 -219");
    runIt("${sesc_parms} -k0x80000 ${marks}","${executable} ${BHOME}/CINT2000/176.gcc/${dataset}/200.i -o gcc1.s", "${BHOME}/CINT2000/176.gcc/${dataset}/200.i",$baseoutput, $param{bench});
  }elsif( $param{bench} eq 'mcf' ) {
    print "Running mcf\n";

    my $marks = getMarks("-11 -26", "-11 -250");
    runIt("${sesc_parms} -h0x6000000 ${marks}","${executable} ${BHOME}/CINT2000/181.mcf/${dataset}/inp.in", "",  $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'crafty' ) {
    print "Running crafty\n";

    my $marks = getMarks("-18 -212", "-18 -215");
    runIt("${sesc_parms} -h0x800000 ${marks}","${executable}", "${BHOME}/CINT2000/186.crafty/${dataset}/crafty.in",  $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'eon' ) {
    print "Running eon\n";

    my $common= "${BHOME}/CINT2000/252.eon/${dataset}/";
    my $marks = getMarks("", "");
    runIt("${sesc_parms} -h0x800000 ${marks}","${executable} ${common}/chair.control.cook ${common}/chair.camera ${common}/chair.surfaces eon.out eon.out 32", "", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'parser' ) {
    print "Running parser\n";

    my $common= "${BHOME}/CINT2000/197.parser/data/all/input/2.1.dict -batch";
    my $marks = getMarks("-135 -238", "-133 -238");
    runIt("${sesc_parms} -k0x80000 -h0x3C00000 ${marks}","${executable} ${common}", "${BHOME}/CINT2000/197.parser/${dataset}/${op_data}.in", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'gap' ) {
    print "Running gap\n";

    my $marks = getMarks("-1122 -2123", "-1114 -2126");
    my $opt = "-l ${BHOME}/CINT2000/254.gap/data/all/input  -q -n -m ";
    if ($op_data eq 'test') {
      $opt .= "64M";
    } elsif ($op_data eq 'train') {
      $opt .= "128M";
    } else {
      $opt .= "128M";
    }
    runIt("${sesc_parms} -h0xC000000 -k0x200000 ${marks}","${executable} ${opt}",
          "${BHOME}/CINT2000/254.gap/${dataset}/${op_data}.in", $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'vortex' ) {
    print "Running vortex\n";

    my $marks = getMarks("-11 -22", "-11 -22");
    my $input;
    # prepare input data then run
    if ($op_data eq 'ref') {
      system("cp ${BHOME}/CINT2000/255.vortex/data/ref/input/persons.1k .");
      system("cp ${BHOME}/CINT2000/255.vortex/data/ref/input/bendian.rnv .");
      system("cp ${BHOME}/CINT2000/255.vortex/data/ref/input/bendian.wnv .");
      $input = "bendian2.raw";
    }elsif ($op_data eq 'train') {
      system("cp ${BHOME}/CINT2000/255.vortex/data/train/input/persons.250 .");
      system("cp ${BHOME}/CINT2000/255.vortex/data/train/input/bendian.rnv .");
      system("cp ${BHOME}/CINT2000/255.vortex/data/train/input/bendian.wnv .");
      $input = "bendian.raw";
    }elsif ($op_data eq 'test') {
      system("cp ${BHOME}/CINT2000/255.vortex/data/test/input/persons.1k .");
      system("cp ${BHOME}/CINT2000/255.vortex/data/test/input/bendian.rnv .");
      system("cp ${BHOME}/CINT2000/255.vortex/data/test/input/bendian.wnv .");
      $input = "bendian.raw";
    }
    runIt("${sesc_parms} -h0x8000000 ${marks}","${executable} ${BHOME}/CINT2000/255.vortex/${dataset}/${input}", "",  $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'bzip2' ) {
    print "Running bzip2\n";

    my $marks = getMarks("-13 -26", "-11 -29");
    my $input;
    if ($op_data eq 'test') {
      $input = "input.random 2";
    } elsif ($op_data eq 'train') {
      $input = "input.compressed 8";
    } else {
      $input = "input.source 58";
    }
    runIt("${sesc_parms} -h0xbc00000 ${marks}","${executable} ${BHOME}/CINT2000/256.bzip2/${dataset}/${input}", "",  $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'twolf' ) {
    print "Running twolf\n";

    my $marks = getMarks("-12 -23", "-12 -23");
    runIt("${sesc_parms} -h0x8000000 ${marks}","${executable} ${op_data}", "",  $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'perlbmk' ) {
    print "Running perlbmk\n";

    my $marks = getMarks("-11 -22", "-11 -22");
    my $opt;
    if ($op_data eq 'ref') {
      system("cp ${BHOME}/CINT2000/253.perlbmk/data/all/input/lenums .");
      $opt = "-Ilib ${BHOME}/CINT2000/253.perlbmk/data/all/input/diffmail.pl 2 550 15 24 23 100";
    }
    runIt("${sesc_parms} -h0x8000000 ${marks}","${executable} ${opt}", "",  $baseoutput, $param{bench});
  }
####################################################################################
#  SPEC FP
####################################################################################
  elsif( $param{bench} eq 'wupwise' ) {
    print "Running wupwise\n";

    my $marks = getMarks("-11 -22", "-11 -24");
    system("cp ${BHOME}/CFP2000/168.wupwise/${dataset}/wupwise.in .");
    runIt("${sesc_parms} -h0xbc00000 ${marks}","${executable}", "",  $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'swim' ) {
    print "Running swim\n";

    my $marks = getMarks("-12 -24", "-12 -26");
    runIt("${sesc_parms} -h0xbc00000 ${marks}","${executable}",
          "${BHOME}/CFP2000/171.swim/${dataset}/swim.in", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'mgrid' ) {
    print "Running mgrid\n";

    my $marks = getMarks("-11 -22", "-11 -23");
    runIt("${sesc_parms} -h0xbc00000 ${marks}","${executable}", $param{bench}, 
          "${BHOME}/CFP2000/172.mgrid/${dataset}/mgrid.in", $baseoutput);
  }elsif( $param{bench} eq 'applu' ) {
    print "Running applu\n";

    my $marks = getMarks("-11 -2110", "-11 -2220");
    runIt("${sesc_parms} -h0xb000000  -k0x20000 ${marks}","${executable}", $param{bench}, 
          "${BHOME}/CFP2000/173.applu/${dataset}/applu.in", $baseoutput);
  }elsif( $param{bench} eq 'mesa' ) {
    print "Running mesa\n";

    my $params;
    my $marks = getMarks("-11 -22", "-11 -23");
    system("cp ${BHOME}/CFP2000/177.mesa/${dataset}/numbers .");
    if ($op_data eq 'test') {
      $params = "-frames 10 -meshfile ${BHOME}/CFP2000/177.mesa/${dataset}/mesa.in -ppmfile mesa.ppm ";
    } elsif ($op_data eq 'train') {
      $params = "-frames 500 -meshfile ${BHOME}/CFP2000/177.mesa/${dataset}/mesa.in -ppmfile mesa.ppm";
    } elsif ($op_data eq 'ref') {
      $params = "-frames 1000 -meshfile ${BHOME}/CFP2000/177.mesa/${dataset}/mesa.in -ppmfile mesa.ppm";
    }
    runIt("${sesc_parms} -k0x80000  -h0x8000000 ${marks}","${executable} ${params}", "", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'galgel' ) {
    print "Running galgel\n";

    # still needs checking - all input sets
    runIt("echo \" facerec: benchmark in f90, need to find parameters. run may be wrong\"", $param{bench});
    runIt("${sesc_parms} -h0xbc00000","${executable}", "${BHOME}/CFP2000/178.galgel/${dataset}/galgel.in", $baseoutput);

  }elsif( $param{bench} eq 'art' ) {
    print "Running art\n";

    my $params;
    my $marks = getMarks("-12 -23", "-12 -25");
    if ($op_data eq 'test') {
      $params = "-stride 2 -startx 134 -starty 220 -endx 139 -endy 225 -objects 1";
    } elsif ($op_data eq 'train') {
      $params = "-stride 2 -startx 134 -starty 220 -endx 184 -endy 240 -objects 3";
    } elsif ($op_data eq 'ref') {
      $params = "-trainfile2 ${BHOME}/CFP2000/179.art/${dataset}/hc.img";
      $params .= "-stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10";
      # --OR-- (spec runs both)
      # $params .= " -stride 2 -startx 470 -starty 140 -endx 520 -endy 180 -objects 10";
    }
  }elsif( $param{bench} eq 'equake' ) {
    print "Running equake\n";

    my $marks = getMarks("-13 -25", "-13 -213");
    runIt("${sesc_parms} -h0xbc00000 ${marks}","${executable}", "${BHOME}/CFP2000/183.equake/${dataset}/inp.in", $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'facerec' ) {
    print "Running facerec\n";

    # still needs checking - all input sets
    runIt("echo \" facerec: benchmark in f90, need to find parameters\"", $param{bench});

  }elsif( $param{bench} eq 'ammp' ) {
    print "Running ammp\n";

    my $marks = getMarks("-1133 -210000", "-1133 -222650");
    if ($op_data eq 'test') {
      system("cp ${BHOME}/CFP2000/188.ammp/${dataset}/all.init.ammp .");
      system("cp ${BHOME}/CFP2000/188.ammp/${dataset}/short.tether .");
    } elsif ($op_data eq 'train') {
      system("cp ${BHOME}/CFP2000/188.ammp/${dataset}/all.new.ammp .");
      system("cp ${BHOME}/CFP2000/188.ammp/${dataset}/new.tether .");
    } elsif ($op_data eq 'ref') {
      system("cp ${BHOME}/CFP2000/188.ammp/${dataset}/all.init.ammp .");
      system("cp ${BHOME}/CFP2000/188.ammp/${dataset}/init_cond.run.1 .");
      system("cp ${BHOME}/CFP2000/188.ammp/${dataset}/init_cond.run.2 .");
      system("cp ${BHOME}/CFP2000/188.ammp/${dataset}/init_cond.run.3 .");
    }
    runIt("${sesc_parms} -h0xc000000 ${marks}","${executable}", "${BHOME}/CFP2000/188.ammp/${dataset}/ammp.in", $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'lucas' ) {
    print "Running lucas\n";

    # still needs checking - all input sets
    runIt("echo \" lucas: benchmark in f90, need to find parameters\"", $param{bench});

  }elsif( $param{bench} eq 'fma3d' ) {
    print "Running fma3d\n";

    # still needs checking - all input sets
    runIt("echo \" fma3d: benchmark in f90, need to find parameters\"", $param{bench});

  }elsif( $param{bench} eq 'sixtrack' ) {
    print "Running sixtrack\n";

    system("cp ${BHOME}/CFP2000/200.sixtrack/data/all/input/fort.2 .");
    system("cp ${BHOME}/CFP2000/200.sixtrack/${dataset}/fort.3 .");
    system("cp ${BHOME}/CFP2000/200.sixtrack/${dataset}/fort.7 .");
    system("cp ${BHOME}/CFP2000/200.sixtrack/${dataset}/fort.8 .");
    system("cp ${BHOME}/CFP2000/200.sixtrack/data/all/input/fort.16 .");
    runIt("${sesc_parms} -k0x80000 -h0x8000000","${executable}", "${BHOME}/CFP2000/200.sixtrack/${dataset}/inp.in", $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'apsi' ) {
    print "Running apsi\n";

    system("cp ${BHOME}/CFP2000/301.apsi/${dataset}/apsi.in .");
    runIt("${sesc_parms} -h0xbc00000","${executable}", "", $baseoutput, $param{bench});

  }

  ############################################################
  # SMTmix - both CINT and CFP together
  ############################################################

  elsif( $param{bench} eq 'wupwisemcf' ) {
    print "Running wupwisemcf\n";

    my $mcfMarks = getMarks("-11 -26", "-11 -250");
    my $wwMarks = getMarks("-11 -22", "-11 -24");
    my $marks = "-m181 $mcfMarks -m168 $wwMarks";

    my $mcfParm = "--begin=mcf ${BHOME}/CINT2000/181.mcf/${dataset}/inp.in";
    my $wwParm = "--begin=wupwise";

    system("cp ${BHOME}/CFP2000/168.wupwise/${dataset}/wupwise.in .");
    runIt("${sesc_parms} -h0x10000000 $marks","${executable} $mcfParm $wwParm", "", $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'mcfart' ) {
    print "Running mcfart\n";

    my $mcfMarks = getMarks("-11 -26", "-11 -250");
    my $artMarks = getMarks("-12 -23", "-12 -25");

    my $marks = "-m181 $mcfMarks -m179 $artMarks";

    my $mcfParm = "${BHOME}/CINT2000/181.mcf/${dataset}/inp.in";

    my $artParm;
    if ($op_data eq 'test') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 139 -endy 225 -objects 1";
    } elsif ($op_data eq 'train') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 184 -endy 240 -objects 3";
    } elsif ($op_data eq 'ref') {
      $artParm = "-trainfile2 ${BHOME}/CFP2000/179.art/${dataset}/hc.img";
      $artParm = "-stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10";
    }

    $artParm = "-scanfile ${BHOME}/CFP2000/179.art/${dataset}/c756hel.in -trainfile1 ${BHOME}/CFP2000/179.art/${dataset}/a10.img $artParm";

    runIt("${sesc_parms} -h0x20000000 ${marks}","${executable} --begin=mcf $mcfParm --begin=art $artParm", "", $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'artequake' ) {
    print "Running artequake\n";

    my $equakeMarks = getMarks("-13 -25", "-13 -213");
    my $artMarks = getMarks("-12 -23", "-12 -25");

    my $marks = "-m183 $equakeMarks -m179 $artMarks";

    my $artParm;
    if ($op_data eq 'test') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 139 -endy 225 -objects 1";
    } elsif ($op_data eq 'train') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 184 -endy 240 -objects 3";
    } elsif ($op_data eq 'ref') {
      $artParm = "-trainfile2 ${BHOME}/CFP2000/179.art/${dataset}/hc.img";
      $artParm = "-stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10";
    }

    $artParm = "-scanfile ${BHOME}/CFP2000/179.art/${dataset}/c756hel.in -trainfile1 ${BHOME}/CFP2000/179.art/${dataset}/a10.img $artParm";

    runIt("${sesc_parms} -h0x20000000 ${marks}","${executable} --begin=equake --begin=art $artParm",
          "${BHOME}/CFP2000/183.equake/${dataset}/inp.in", $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'craftyperlbmk' ) {
    print "Running craftyperlbmk\n";

      my $craftyMarks = getMarks("-18 -212", "-18 -215");
      my $perlbmkMarks = getMarks("-11 -22", "-11 -22");
      
      my $marks = "-m186 $craftyMarks -m253 $perlbmkMarks";
      
      #for perlbmk
      my $perlbmkParm;
      if ($op_data eq 'ref') {
          system("cp ${BHOME}/CINT2000/253.perlbmk/data/all/input/lenums .");
          $perlbmkParm = "-Ilib ${BHOME}/CINT2000/253.perlbmk/data/all/input/diffmail.pl 2 550 15 24 23 100";
      }

      runIt("${sesc_parms} -h0x10000000  ${marks}","${executable} --begin=crafty --begin=perlbmk $perlbmkParm", 
            "${BHOME}/CINT2000/186.crafty/${dataset}/crafty.in", $baseoutput, $param{bench});
  }


  elsif( $param{bench} eq 'mesaart' ) {
    print "Running mesaart\n";

    my $mesaMarks = getMarks("-11 -22", "-11 -23");
    my $artMarks = getMarks("-12 -23", "-12 -25");

    my $marks = "-m177 $mesaMarks -m179 $artMarks";

    #parms for mesa
    my $mesaParm;
    system("cp ${BHOME}/CFP2000/177.mesa/${dataset}/numbers .");
    if ($op_data eq 'test') {
      $mesaParm = "-frames 10 -meshfile ${BHOME}/CFP2000/177.mesa/${dataset}/mesa.in -ppmfile mesa.ppm ";
    } elsif ($op_data eq 'train') {
      $mesaParm = "-frames 500 -meshfile ${BHOME}/CFP2000/177.mesa/${dataset}/mesa.in -ppmfile mesa.ppm";
    } elsif ($op_data eq 'ref') {
      $mesaParm = "-frames 1000 -meshfile ${BHOME}/CFP2000/177.mesa/${dataset}/mesa.in -ppmfile mesa.ppm";
    }

    #parms for art
    my $artParm;
    if ($op_data eq 'test') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 139 -endy 225 -objects 1";
    } elsif ($op_data eq 'train') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 184 -endy 240 -objects 3";
    } elsif ($op_data eq 'ref') {
      $artParm = "-trainfile2 ${BHOME}/CFP2000/179.art/${dataset}/hc.img";
      $artParm = "-stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10";
    }
    $artParm = "-scanfile ${BHOME}/CFP2000/179.art/${dataset}/c756hel.in -trainfile1 ${BHOME}/CFP2000/179.art/${dataset}/a10.img $artParm";

    runIt("${sesc_parms}  -k0x80000 -h0x20000000 ${marks}","${executable} --begin=mesa $mesaParm --begin=art $artParm", "", $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'mcfparser' ) {
    print "Running mcfparser\n";

    my $mcfMarks = getMarks("-11 -26", "-11 -250");
    my $parserMarks = getMarks("-135 -238", "-133 -238");

    my $marks = "-m181 $mcfMarks -m197 $parserMarks";

    my $mcfParm = "${BHOME}/CINT2000/181.mcf/${dataset}/inp.in";
    my $parserParm = "${BHOME}/CINT2000/197.parser/data/all/input/2.1.dict -batch ";

    runIt("${sesc_parms} -k0x80000 -h0x20000000 ${marks}","${executable} --begin=mcf $mcfParm --begin=parser $parserParm", 
          "${BHOME}/CINT2000/197.parser/${dataset}/${op_data}.in",  $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'wupwiseperlbmk' ) {
    print "Running wupwiseperlbmk\n";

    my $wwMarks = getMarks("-11 -22", "-11 -24");
    my $perlbmkMarks = getMarks("-11 -22", "-11 -22");
    my $marks = "-m168 $wwMarks -m253 $perlbmkMarks";

    #for perlbmk
    my $perlbmkParm;
    if ($op_data eq 'ref') {
        system("cp ${BHOME}/CINT2000/253.perlbmk/data/all/input/lenums .");
          $perlbmkParm = "-Ilib ${BHOME}/CINT2000/253.perlbmk/data/all/input/diffmail.pl 2 550 15 24 23 100";
    }
    
    system("cp ${BHOME}/CFP2000/168.wupwise/${dataset}/wupwise.in .");
    runIt("${sesc_parms} -h0x10000000 $marks","${executable} --begin=wupwise  --begin=perlbmk $perlbmkParm", "",  $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'bzip2vpr' ) {
    print "Running bzip2vpr\n";

    my $bzipMarks = getMarks("-11 -22", "-11 -22");
    my $vprMarks = getMarks("-11 -22", "-11 -22");

    my $marks = "-m256 $bzipMarks -m175 $vprMarks";

    my $bzipParm = "${BHOME}/CINT2000/256.bzip2/${dataset}/";

    if ($op_data eq 'test') {
      $bzipParm .= "input.random 2";
    } elsif ($op_data eq 'train') {
      $bzipParm .= "input.compressed 8";
    } else {
      $bzipParm .= "input.source 58";
    }

    my $vprParm = "${BHOME}/CINT2000/175.vpr/${dataset}/net.in ";
    $vprParm .= "${BHOME}/CINT2000/175.vpr/${dataset}/arch.in ";
    $vprParm .= "place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 ";
    $vprParm .= "-alpha_t 0.9412 -inner_num 2";

    runIt("${sesc_parms} -h0x10000000 ${marks}","${executable} --begin=bzip2 $bzipParm --begin=vpr $vprParm","", $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'swimmcf' ) {
    print "Running swimmcf\n";

    my $swimMarks = getMarks("-12 -24", "-12 -26");
    my $mcfMarks = getMarks("-11 -26", "-11 -250");
    my $marks = "-m181 $mcfMarks -m171 $swimMarks";

    my $mcfParm = "--begin=mcf ${BHOME}/CINT2000/181.mcf/${dataset}/inp.in";
    my $swimParm = "--begin=swim";

    runIt("${sesc_parms} -h0x10000000 $marks","${executable} $mcfParm $swimParm", 
          "${BHOME}/CFP2000/171.swim/${dataset}/swim.in",  $baseoutput, $param{bench});
  }
  
  elsif( $param{bench} eq 'mgridmcf' ) {
    print "Running mgridmcf\n";

    my $mgridMarks = getMarks("-11 -22", "-11 -23");
    my $mcfMarks = getMarks("-11 -26", "-11 -250");
    my $marks = "-m181 $mcfMarks -m172 $mgridMarks";

    my $mcfParm = "--begin=mcf ${BHOME}/CINT2000/181.mcf/${dataset}/inp.in";
    my $mgridParm = "--begin=mgrid";

    runIt("${sesc_parms} -h0x10000000 $marks","${executable} $mcfParm $mgridParm", 
          "${BHOME}/CFP2000/172.mgrid/${dataset}/mgrid.in",  $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'equakeswim' ) {
    print "Running equakeswim\n";

    my $equakeMarks = getMarks("-13 -25", "-13 -213");
    my $swimMarks = getMarks("-12 -24", "-12 -26");

    my $marks = "-m183 $equakeMarks -m171 $swimMarks";

    my $swimParm = "--begin=swim";

    runIt("${sesc_parms} -h0x20000000 ${marks}","${executable} --begin=equake $swimParm", 
          "${BHOME}/CFP2000/171.swim/${dataset}/swim.in", $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'equakeperlbmk' ) {
    print "Running equakeperlbmk\n";

    my $equakeMarks = getMarks("-13 -25", "-13 -213");
    my $perlbmkMarks = getMarks("-11 -22", "-11 -22");

    my $marks = "-m183 $equakeMarks -m253 $perlbmkMarks";

    #for perlbmk
    my $perlbmkParm = "--begin=perlbmk ";
    if ($op_data eq 'ref') {
        system("cp ${BHOME}/CINT2000/253.perlbmk/data/all/input/lenums .");
        $perlbmkParm .= "-Ilib ${BHOME}/CINT2000/253.perlbmk/data/all/input/diffmail.pl 2 550 15 24 23 100";
    }

    my $equakeParm = "--begin=equake";

    runIt("${sesc_parms} -h0x18000000  ${marks}","${executable} $perlbmkParm $equakeParm", "${BHOME}/CFP2000/183.equake/${dataset}/inp.in", $baseoutput, $param{bench});
  }  

  elsif( $param{bench} eq 'artgap' ) {
    print "Running artgap\n";

    my $artMarks = getMarks("-12 -23", "-12 -25");
    my $gapMarks = getMarks("-1122 -2123", "-1141 -2145");

    my $marks = "-m179 $artMarks -m254 $gapMarks";

    #for art
    my $artParm;
    if ($op_data eq 'test') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 139 -endy 225 -objects 1";
    } elsif ($op_data eq 'train') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 184 -endy 240 -objects 3";
    } elsif ($op_data eq 'ref') {
      $artParm = "-trainfile2 ${BHOME}/CFP2000/179.art/${dataset}/hc.img";
      $artParm = "-stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10";
    }

    $artParm = "--begin=art -scanfile ${BHOME}/CFP2000/179.art/${dataset}/c756hel.in -trainfile1 ${BHOME}/CFP2000/179.art/${dataset}/a10.img $artParm";

    #for gap
    my $gapParm = "--begin=gap -l ${BHOME}/CINT2000/254.gap/data/all/input  -q -n -m ";
    if ($op_data eq 'test') {
      $gapParm .= "64M";
    } elsif ($op_data eq 'train') {
      $gapParm .= "128M";
    } else {
      $gapParm .= "128M";
    }

    runIt("${sesc_parms} -h0x20000000 ${marks}","${executable} $artParm $gapParm", 
          "${BHOME}/CINT2000/254.gap/${dataset}/${op_data}.in",  $baseoutput, $param{bench});
  }

  elsif( $param{bench} eq 'artperlbmk' ) {
    print "Running artperlbmk\n";

    my $artMarks = getMarks("-12 -23", "-12 -25");
    my $perlbmkMarks = getMarks("-11 -22", "-11 -22");

    my $marks = "-m179 $artMarks -m253 $perlbmkMarks";

    #for art
    my $artParm;
    if ($op_data eq 'test') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 139 -endy 225 -objects 1";
    } elsif ($op_data eq 'train') {
      $artParm = "-stride 2 -startx 134 -starty 220 -endx 184 -endy 240 -objects 3";
    } elsif ($op_data eq 'ref') {
      $artParm = "-trainfile2 ${BHOME}/CFP2000/179.art/${dataset}/hc.img";
      $artParm = "-stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10";
    }

    $artParm = "--begin=art -scanfile ${BHOME}/CFP2000/179.art/${dataset}/c756hel.in -trainfile1 ${BHOME}/CFP2000/179.art/${dataset}/a10.img $artParm";

    #for perlbmk
    my $perlbmkParm = "--begin=perlbmk ";
    if ($op_data eq 'ref') {
        system("cp ${BHOME}/CINT2000/253.perlbmk/data/all/input/lenums .");
        $perlbmkParm .= "-Ilib ${BHOME}/CINT2000/253.perlbmk/data/all/input/diffmail.pl 2 550 15 24 23 100";
    }

    runIt("${sesc_parms} -h0x20000000 ${marks}","${executable} $artParm $perlbmkParm", "",  $baseoutput, $param{bench});
  }

  
  ###########################################################
  #  splash2
  ###########################################################

  elsif( $param{bench} eq 'cholesky' ) {
    print "Running cholesky\n";

    my $params;
    if ($op_data eq 'ref') {
      $params = "-p${op_numprocs} ${BHOME}/splash2/kernels/cholesky/${dataset}/tk15.O"
    } elsif ($op_data eq 'test') {
      $params = "-p${op_numprocs} ${BHOME}/splash2/kernels/cholesky/${dataset}/lshp.O"
    }
    runIt("${sesc_parms} -h0x8000000 -k0x80000","${executable} ${params}", "",  $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'fft' ) {
    print "Running fft\n";

    my $params;
    if ($op_data eq 'ref') {
      $params = "-m16 -l5 -p${op_numprocs}";
    } elsif ($op_data eq 'test') {
      $params = "-m12 -l5 -p${op_numprocs}";
    }
    runIt("${sesc_parms} -h0x8000000","${executable} ${params}", "", $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'lu' ) {
    print "Running lu\n";

    my $params;
    if ($op_data eq 'ref') {
      $params = "-n512 -b16 -p${op_numprocs}";
    } elsif ($op_data eq 'test') {
      $params = "-n32 -b8 -p${op_numprocs}";
    }
    runIt("${sesc_parms} -h0x8000000","${executable} ${params}", "",  $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'radix' ) {
    print "Running radix\n";

    my $params;
    if ($op_data eq 'ref') {
      $params = "-r1024 -n1048576 -p${op_numprocs}";
    } elsif ($op_data eq 'test') {
      $params = "-r32 -n65536 -p${op_numprocs}";
    }
    runIt("${sesc_parms} -h0x8000000","${executable} ${params}", "", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'barnes' ) {
    print "Running barnes\n";

    runIt("${sesc_parms} -h0x8000000","${executable}", "${BHOME}/splash2/apps/barnes/${dataset}/i${op_numprocs}", $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'fmm' ) {
    print "Running fmm\n";

   runIt("${sesc_parms} -h0x8000000","${executable}", "${BHOME}/splash2/apps/fmm/${dataset}/i16kp${op_numprocs}", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'ocean' ) {
    print "Running ocean\n";

    my $params;
    if ( $op_data eq 'ref') {
      $params = "-n258 -p${op_numprocs}";
    } else {
      $params = "-n34 -p${op_numprocs}";
    }
    runIt("${sesc_parms} -h0x8000000","${executable} ${params}", "", $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'radiosity' ) {
    print "Running radiosity\n";

    my $params;
    if ($op_data eq 'ref') {
      $params = "-batch -room -ae 5000.0 -en 0.050 -bf 0.10 -p ${op_numprocs}"
    } elsif ($op_data eq 'test') {
      $params = "-batch -room -ae 5000.0 -en 0.050 -bf 0.10 -p ${op_numprocs}";
    }
    runIt("${sesc_parms} -h0x8000000","${executable} ${params}", "", $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'raytrace' ) {
    print "Running raytrace\n";

    my $params;
    if ($op_data eq 'ref') {
      system("cp ${BHOME}/splash2/apps/raytrace/${dataset}/car.geo .");
      $params = "-m64 -p${op_numprocs} ${BHOME}/splash2/apps/raytrace/${dataset}/car.env";
    } elsif ($op_data eq 'test') {
      system("cp ${BHOME}/splash2/apps/raytrace/${dataset}/teapot.geo .");
      $params = "-m64 -p${op_numprocs} ${BHOME}/splash2/apps/raytrace/${dataset}/teapot.env";
    }
    runIt("${sesc_parms} -h0x8000000","${executable} ${params}", "",  $baseoutput, $param{bench});
  }elsif( $param{bench} eq 'volrend' ) {
    print "Running volrend\n";

    my $params;
    if ($op_data eq 'ref') {
      $params = "${op_numprocs} ${BHOME}/splash2/apps/volrend/${dataset}/head";
    } elsif($op_data eq 'test') {
      $params = "${op_numprocs} ${BHOME}/splash2/apps/volrend/${dataset}/head-scaleddown4";
    }
    runIt("${sesc_parms} -h0x8000000","${executable} ${params}", "", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'water-nsquared' ) {
    print "Running water-nsquared\n";

    system("cp ${BHOME}/splash2/apps/water-nsquared/${dataset}/random.in .");
    runIt("${sesc_parms} -h0x8000000","${executable} ${BHOME}/splash2/apps/water-nsquared/${dataset}/input_${op_numprocs}", "", $baseoutput, $param{bench});

  }elsif( $param{bench} eq 'water-spatial' ) {
    print "Running water-spatial\n";

    system("cp ${BHOME}/splash2/apps/water-spatial/${dataset}/random.in .");
    runIt("${sesc_parms} -h0x8000000","${executable} ${BHOME}/splash2/apps/water-spatial/${dataset}/input_${op_numprocs}", "", $baseoutput, $param{bench});
  }else{
    die("Unknown benchmark [$param{xtra}]");
  }
}

sub cleanAll {
      unlink <core.*>;
      unlink <sesc_*\.??????>;
      unlink <prof_*>;
      unlink 'run.pl.log';
      unlink 'game.001';
      unlink <crafty*>;
      unlink <AP*>;
      unlink 'mcf.out';
      unlink 'smred.msg';
      unlink 'smred.out';
      unlink 'lgred.msg';
      unlink 'lgred.out';
      unlink 'bendian.rnv';
      unlink 'bendian.wnv';
      unlink 'persons.1k';
      unlink 'lenums';
      unlink 'vortex1.out';
      unlink 'vortex2.out';
      unlink 'eon.out';
      unlink 'eon.dat';
      unlink 'materials';
      unlink 'spectra.dat';
      unlink 'vortex.msg';
      unlink 'lib';
      unlink <fort*>;
      unlink <input/*> ;
      rmdir 'input' ;
      unlink <words/CVS/*>;
      rmdir 'words/CVS';
      unlink <words/*>;
      rmdir 'words' ;
      unlink 'wupwise.in';
      unlink 'SWIM7';
      unlink 'costs.out';
      unlink '166.i';
      unlink '200.i';
      unlink 'place.out';
      unlink 'all.init.ammp';
      unlink 'init_cond.run.1';
      unlink 'init_cond.run.2';
      unlink 'init_cond.run.3';
      unlink 'a10.img';
      unlink 'c756hel.in';
      unlink 'hc.img';

      unlink 'numbers';
      unlink 'mesa.in';
      unlink 'mesa.log';
      unlink 'mesa.ppm';

      system("rm -rf lgred");
      system("rm -rf smred");
      unlink 'benums' ;

      unlink 'gcc1.s';
      unlink 'apsi.in';

      unlink <test.*>;
      unlink <train.*>;
      unlink <ref.*>;
}

sub processParams {
  my $badparams =0;

  $badparams = 1 if( @ARGV < 1 );

  if ( -f "./${op_sesc}" ) {
     $op_sesc = "./${op_sesc}";
  }else{
     $op_sesc = $op_sesc;
  }

  if( $op_clean ) {
    if ( $op_yes) {
      cleanAll();
    }else{
      print "Do you really want to DELETE all the files? (y/N)";
      my $c = getc();
      if( $c eq 'y' ) {
        cleanAll();
      }
    }
    exit 0;
  }

  if( defined ($op_bhome) ) {
      $BHOME = $op_bhome;
  }else{
      print "Path to benchmarks directory undefined. Use bhome option\n";
      $badparams =1;
  }

  if( !defined ($op_bindir) ) {
      print "Path to benchmarks binary undefined. Use bindir option\n";
      $badparams =1;
  }

  if ($op_data eq "test" || $op_data eq "ref" || $op_data eq "train") {
    $dataset  = "data/";
    $dataset .= $op_data;
    $dataset .= "/input";
  } else {
    print "Choose dataset to run: [test|ref|train]\n";
    $badparams =1;
  }

  if (defined $ENV{'email'}) {
    $op_email = $ENV{'email'};
  }else{
    $op_email = $ENV{'USER'};
    my $ldap_txt = `ldapsearch -x -LLL -u -t \"(uid=\${USER})\" mail | grep ^mail`;
    if ( $ldap_txt =~ /mail:\ (.+)/ ) {
      $op_email = $1;
    }
  }

  if( $badparams or $op_help ){
    print "usage:\n\trun.pl <options> <benchs>*\n";
    print "CINT2000: crafty mcf parser gzip vpr bzip2 gcc gap twolf vortex perlbmk eon\n";
    print "CFP2000 : wupwise swim mgrid applu apsi equake ammp art mesa\n";
    print "SPLASH2: cholesky fft lu radix barnes fmm ocean radiosity  \n";
    print "         raytrace volrend water-nsquared water-spatial     \n";
    print "SMTmix: wupwisemcf\n";
    print "Misc   : mp3dec mp3enc smatrix\n";
    print "\t-sesc=s          ; Simulator executable (default is $ENV{'SESCBUILDDIR'}/sesc)\n";
    print "\t-c=s             ; Configuration file (default is $op_c).\n";
    print "\t-bhome=s         ; Path for the benchmarks directory is located (default is $op_bhome)\n";
    print "\t-bindir=s        ; Specify where the benchmarks binaries are (deafult is $op_bindir).\n";
    print "\t-ext=s           ; Extension to be added to the binary names\n";
    print "\t-data=s          ; Data set [test|train|ref]\n";
    print "\t-test            ; Just test the configuration\n";
    print "\t-key=s           ; Extra key added to result file name\n";
    print "\t-load=i          ; # simultaneous simulations running\n";
    print "\t-mload=i         ; Maximum machine load\n";
    print "\t-prof=i          ; Profiling run\n";
    print "\t-profsec=s       ; Profiling section\n";
    print "\t-fast            ; Shorter marks for fast test\n";
    print "\t-procs=i         ; Number of threads in a splash application\n";
    print "\t-ninst=i         ; Maximum number of instructions to simulate\n";
    print "\t-spoint=i        ; Simulation point start (10M inst)\n";
    print "\t-spointsize=i    ; Simulation point size (100M or 10M inst)\n";
    print "\t-rabbit          ; Run in Rabbit mode the whole program\n";
    print "\t-full            ; Run the whole program to completionm\n";
    print "\t-saveoutput      ; Save output to a .out file\n";
    print "\t-clean           ; DELETE all the outputs from previous runs\n";
    print "\t-condor          ; Submit jobs using condor.\n";
    print "\t-native          ; native execution.\n";
    print "\t-condorstd       ; Use condor standard universe.\n";
    print "\t-qsub            ; Use SUN Grid queue system.\n";
    print "\t-email=s         ; SUN Grid job termination/cancellation email.\n";
    print "\t-yes             ; Do not ask for questions. Say all yes\n";
    print "\t-trace           ; Interpret benchmarks as trace files\n";
    print "\t-help            ; Show this help\n";
    exit;
  }
}

sub setupDirectory {
  return if ($op_trace);  

  unless( -d "words") {
      system("mkdir words");
      system("cp ${BHOME}/CINT2000/197.parser/data/all/input/words/* words/");
  }
  system("cp ${BHOME}/CINT2000/300.twolf/${dataset}/${op_data}.* .");
  system("ln -sf ${BHOME}/CINT2000/253.perlbmk/data/all/input/lib lib") unless ( -l "lib");
  system("cp ${BHOME}/CINT2000/252.eon/data/ref/input/eon.dat .");
  system("cp ${BHOME}/CINT2000/252.eon/data/ref/input/materials .");
  system("cp ${BHOME}/CINT2000/252.eon/data/ref/input/spectra.dat .");
}

exit &main();

###########

sub main {

  processParams();

  my $tmp;
  my $bench;

  open( RUNLOG, ">>run.pl.log") or die("Can't open log file");

  print RUNLOG "-----------------------------------------\n";
  print RUNLOG ctime(time()) . "   :   ";
  print RUNLOG `uname -a`;
  print RUNLOG "-----------------------------------------\n";

  setupDirectory();

  if($op_condor) { 
      open($jobfp, ">condorjob");

      system("mkdir -p condorout");
      printf $jobfp "Executable   = $op_sesc\n";
      printf $jobfp "Image_Size = 150000\n";
      printf $jobfp "Requirements = Memory > 200 && Machine != \"iacoma38.cs.uiuc.edu\"\n";
      if($op_condorstd) {
          printf $jobfp "Universe     = standard\n";
      } else {
          printf $jobfp "Universe     = vanilla\n";
      }
      printf $jobfp "Notification = Error\n";
      printf $jobfp "\n\n\n";
  }

  foreach $bench (@ARGV) {
      if($op_trace) {
	  runTrace( bench => $bench, xtra => $op_key, xtra2 => $op_key2);
      } else {
	  runBench( bench => $bench , xtra => $op_key, xtra2 => $op_key2);
      }  
  }
  
  if($op_condor) {
      close $jobfp;
      close(RUNLOG);
  }
  
  print "RUN: Spawning finished. Waiting for results...\n" if( $threadsRunning );

  while( $threadsRunning > 0 ) {
    wait();
    $threadsRunning--;
    print "RUN: Another finished. $threadsRunning to complete\n";
  }

  close(RUNLOG);
  exit(0);
}

