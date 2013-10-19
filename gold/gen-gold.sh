#Navigate to the top level directory
cd ../

#Get the path of esesc source and build directory
INPUT=$(head -1 cmake_install.cmake)
esescDir=`echo $INPUT| cut -d':' -f 2`
buildDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

#Run bash script to set up directories and benchmark scripts
sh $esescDir/gold/setup.sh $esescDir

sh $esescDir/gold/exe/maketable.sh SLOW gold $buildDir $esescDir
sh $esescDir/gold/exe/maketable.sh FAST gold $buildDir $esescDir
