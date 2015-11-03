#!/bin/bash 

# $1 is the build directory 
# $2 is the projs/esesc directory
cd $1

if [ ! -d "gold" ]; then
  mkdir gold
fi

cd gold/
cp $1/run/esesc.conf temp.conf 

if [ ! -d "conf" ]; then
  mkdir conf 
fi

#if [ ! -d "gold" ]; then
#  mkdir gold
#fi

if [ ! -d "report" ]; then
  mkdir report
fi

if [ ! -d "tables" ]; then
  mkdir tables
fi

sed 's/#benchName/BBBBBB/g' temp.conf > m1.txt
sed 's/benchName/#benchName/g' m1.txt > m2.txt
sed 's/BBBBBB = "memkernels\/stream"/benchName = "memkernels\/stream"/g' m2.txt > m3.txt
sed 's/BBBBBB/#benchName/g' m3.txt > m4.txt
sed 's/(#benchName)/(benchName)/g' m4.txt > conf/conf_memkernels_stream.txt 
rm m1.txt m2.txt m3.txt m4.txt

sed 's/#benchName/BBBBBB/g' temp.conf > m1.txt
sed 's/benchName/#benchName/g' m1.txt > m2.txt
sed 's/BBBBBB = "memkernels\/stream_omp"/benchName = "memkernels\/stream_omp"/g' m2.txt > m3.txt
sed 's/BBBBBB/#benchName/g' m3.txt > m4.txt
sed 's/(#benchName)/(benchName)/g' m4.txt > conf/conf_memkernels_stream_omp.txt
rm m1.txt m2.txt m3.txt m4.txt

sed 's/#benchName/BBBBBB/g' temp.conf > m1.txt
sed 's/benchName/#benchName/g' m1.txt > m2.txt
sed 's/BBBBBB = "launcher -- blackscholes/benchName = "launcher -- blackscholes/g' m2.txt > m3.txt
sed 's/BBBBBB/#benchName/g' m3.txt > m4.txt
sed 's/(#benchName)/(benchName)/g' m4.txt > conf/conf_launcher_blackscholes.txt
rm m1.txt m2.txt m3.txt m4.txt

sed 's/#benchName/BBBBBB/g' temp.conf > m1.txt
sed 's/benchName/#benchName/g' m1.txt > m2.txt
sed 's/BBBBBB = "launcher -- fft/benchName = "launcher -- fft/g' m2.txt > m3.txt
sed 's/BBBBBB/#benchName/g' m3.txt > m4.txt
sed 's/(#benchName)/(benchName)/g' m4.txt > conf/conf_launcher_fft.txt
rm m1.txt m2.txt m3.txt m4.txt

cp temp.conf conf/conf_launcher_crafty.txt

cd $1/run 

#Crafty is not used in this test since it requires proprietory data .... 
#$1/main/esesc > $1/run/crafty.input
#$2/conf/scripts/report.pl -last > $1/run/Regression-Test/report/report_launcher_crafty.txt

for nom in `seq 1 10`; 
do 
	cp $1/gold/conf/conf_launcher_blackscholes.txt $1/run/esesc.conf
	$1/main/esesc > $1/run/blackscholes.input 
	$2/conf/scripts/report.pl -last > $1/gold/report/report_launcher_blackscholes_$nom.txt
	for f in "launcher_fft" "memkernels_stream" "memkernels_stream_omp"  
	do
	   g='conf_'$f'.txt'
	   cp $1/gold/conf/$g $1/run/esesc.conf
	   sleep 2
	   $1/main/esesc 
	   h='report_'$f'_'$nom'.txt' 
	   $2/conf/scripts/report.pl -last > $1/gold/report/$h
	done
done

mv $1/gold/temp.conf $1/run/esesc.conf 
rm -r $1/gold/conf/ 
python $2/gold/gold-Extract.py $1 $2
