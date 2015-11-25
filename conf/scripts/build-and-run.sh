#!/bin/bash

# Default for ESESC source code location
: ESESC_SRC=${ESESC_SRC:=${HOME}/projs/esesc}

if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "BUILD ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

${ESESC_SRC}/conf/scripts/build.sh
if [ ! $? -eq 0 ]; then
  echo "ESESC build error"
  exit -1
fi

${ESESC_SRC}/conf/scripts/run-test.sh

