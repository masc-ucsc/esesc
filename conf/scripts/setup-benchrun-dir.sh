#!/bin/bash

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <benchdir> <esesc_src>"
  exit -1
fi

BENCH_REPO=$1
ESESC_SRC=$2

ln -s ${BENCH_REPO}/bins bins
ln -s ${BENCH_REPO}/scripts scripts
ln -s ${BENCH_REPO}/data data

cp $ESESC_SRC/conf/*conf* .

