#!/bin/sh
#$ -S /bin/sh
#$ -m es
#$ -M renau@soe.ucsc.edu
#$ -cwd
#$ -o gzip.log
#$ -e gzip.err
#$ -N "gzip"

#./simu/libcore/coremain -c gzip.conf
./simu/libcore/coremain

exit 0
