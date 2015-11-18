# Updated by Hadley Black

# shell file to run the Ethan Jim CMPE202 ESESC regression class project
# directories of note are:
#    /esesc/gold                run this shell file from here
#    /esesc/gold/config         copies of default and modified shared.conf files live here
#    $3/regression/run   esesc shell files run here where the heavy lifting gets done
#    run Process with 'sh Process.sh <shared.conf.(SLOW or FAST or MILD or MEDIUM)>'
#    will output table of /esesc/gold/tables/$1.$date.csv from run in a 9x5 table of benchmarks vs. /esesc/gold/tables/$1.$date.csv for given shared.conf
#    

#$3 = build dir, $4 = esesc dir

# initialize $4/gold/tables/$1.$date.csv output file with current time-stamp
date=`date +%y%m%d-%H%M`

# copy the appropriate shared.conf config file on top of the current working shared.conf file
  cp $4/gold/config/shared.conf.$1 $3/regression/run/shared.conf
  echo -----------------------------------------------------applied shared.conf.$1

# nested loop through all the test-benchmark combinations
# one test at a time, tests as specified in the corresponding shared.conf files
# for each test run all the benchmarks, generating reports and extracting $4/gold/tables/$1.$date.csv for each test

for BENCHMARK in crafty mcf vpr #swim mcf crafty 

do
  # all the ESESC heavy lifting occurs in the ESESC project runs directory
  cd $3/regression/run

  # run one benchmark shell file, ie: crafty _SMARTSMode-run.sh, swim _SMARTSMode-run.sh, ...
  echo -----------------------------------------------------execute $1.$BENCHMARK
  ./$BENCHMARK""_TASS-run.sh
  
  # that generates output in the appropriate sub-directory under $3/regression/run
  # cd to that sub-directory to run a report
  cd $BENCHMARK""_TASS

  # run the report, directing output over to $4/gold/report for processing soon
  $4/conf/scripts/report.pl --last > $3/regression/report/report.$1.$BENCHMARK
done

#Navigate to report in the compiled esesc directory and use the report benchmarks as input
cd $3/regression/report
for BENCHMARK in crafty mcf vpr #swim mcf crafty 
do
  echo $1, Exetime, IPC, uIPC, rawInst, >>$3/regression/tables/$1.$date.$BENCHMARK
  #echo $1, Exetime, IPC, uIPC, rawInst, >>$1.$date.$BENCHMARK

  if [ "$BENCHMARK" = crafty ]; then 
    echo -n crafty,  >>$3/regression/tables/$1.$date.crafty
    #echo -n crafty ,  >>$1.$date.crafty	
       cd $3/regression/tables
       cp $3/regression/report/report.$1.$BENCHMARK report.$1.$BENCHMARK
       python $4/gold/exe/mk_table.py report.$1.$BENCHMARK $1.$date.crafty
       rm report.$1.$BENCHMARK
        
    echo "" >>$3/regression/tables/$1.$date.crafty

  else if [ "$BENCHMARK" = vpr ]; then
    echo -n VPR, >>$3/regression/tables/$1.$date.vpr

       cd $3/regression/tables
       cp $3/regression/report/report.$1.$BENCHMARK report.$1.$BENCHMARK
       python $4/gold/exe/mk_table.py report.$1.$BENCHMARK $1.$date.vpr
       rm report.$1.$BENCHMARK

    echo "" >>$3/regression/tables/$1.$date.vpr

  else if [ "$BENCHMARK" = mcf ]; then        
    echo -n MCF, >>$3/regression/tables/$1.$date.mcf 
    
   
       cd $3/regression/tables
       cp $3/regression/report/report.$1.$BENCHMARK report.$1.$BENCHMARK
       python $4/gold/exe/mk_table.py report.$1.$BENCHMARK $1.$date.mcf
       rm report.$1.$BENCHMARK
    
    echo "" >>$3/regression/tables/$1.$date.mcf
  fi
fi
fi
done

for BENCHMARK in crafty mcf vpr #swim mcf crafty 
do
  cp $3/regression/tables/$1.$date.$BENCHMARK $3/regression/tables/$1.last.$BENCHMARK

  if [ "$2" = gold ]; then
    mv $3/regression/tables/$1.$date.$BENCHMARK $4/gold/tables/$1.gold.$BENCHMARK
    rm $3/regression/tables/$1.last.$BENCHMARK
  fi
done
