#Navigate back to the top level of the ESESC directory.
cd ../

#Get the path of ESESC source and build directory
input=$(head -1 cmake_install.cmake)
esescDir=`echo $input| cut -d':' -f 2`
buildDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

#Run bash script to set up directories and generate benchmark scripts
sh $esescDir/gold/setup.sh $esescDir

#Copy compare table scripts over to compiled esesc dir, we need to generate files there.
cp $esescDir/gold/tables/* $buildDir/regression/tables

# take a known 'golden-brick' output from ESESC and compare against runs of all 3 
# benchmarks this script will invoke
#
# The script will compare a SLOW and FAST run against the.gold.craftyen brick outputs, as well as do a sanity 
# check that the SLOW configuration is indeed slower than the FAST configuration. 
#
#Run ESESC to get new output to compare against.gold.craftyen-brick. Writes output to the compiled
#ESESC directory.
sh $esescDir/gold/exe/report_to_table.sh SLOW NULL $buildDir $esescDir
sh $esescDir/gold/exe/report_to_table.sh FAST NULL $buildDir $esescDir

#Call cmptab-eq(equal) to compare the new outputs against the golden brick should not
#be equal to the golden brick output
cd $buildDir/regression/tables

#crafty and mcf, should combine the two
sh cmptab-eq.sh SLOW.last.crafty SLOW.gold.crafty FAST.last.crafty FAST.gold.crafty \
                SLOW.last.mcf SLOW.gold.mcf FAST.last.mcf FAST.gold.mcf \
                SLOW.last.vpr SLOW.gold.vpr FAST.last.vpr FAST.gold.vpr \
                $esescDir

#now call cmptab-ne.sh for the sanity check of the SLOW vs. FAST conf. These output should
#not be equal but rather should be LT or GT
sh cmptab-ne.sh SLOW.last.crafty FAST.last.crafty \
                SLOW.last.mcf FAST.last.mcf \
                SLOW.last.vpr FAST.last.vpr \
                $esescDir

#copy all output files to the output directory
for BENCHMARK in crafty mcf vpr
do
  mv SLOW.last.$BENCHMARK:SLOW.gold.$BENCHMARK $buildDir/regression/output
  mv FAST.last.$BENCHMARK:FAST.gold.$BENCHMARK $buildDir/regression/output
  mv SLOW.last.$BENCHMARK:FAST.last.$BENCHMARK $buildDir/regression/output
done

#look for ERROR and OK and return message
cd $buildDir/regression/output
wordcount=`grep ERROR * | wc -w`
if [ $wordcount != 0 ]; then
  echo -e "\n"
  echo -e "\n"
  echo -e "\n--------------------------ERRORS FOUND, EXAMINE $buildDir/regression/output------"
else
  echo -e "\n"
  echo -e "\n"
  echo -e "\n-------------------------------------------ALL OK--------------------------------"
fi 

