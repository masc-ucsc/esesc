#!/bin/bash

# Use default parameters or command line args
DOCKER_NAME=$1
PREFIX=${2:-/local/gsouther}
BUILD_TYPE=${3:-Debug}
ESESC_SRC=${4:-$PREFIX/projs/esesc}
ESESC_BUILD_DIR=${5:-$PREFIX/build/docker-esesc}
BENCH_REPO=${6:-$PREFIX/projs/benchmarks}
DOCKER_IMAGE=${7:-mascucsc/criutest}

BUILD_DIR=${ESESC_BUILD_DIR}/${BUILD_TYPE,,}
ESESC_BIN=${BUILD_DIR}/main/esesc
CONF_DIR=${ESESC_SRC}/conf
BENCH_SCRIPT=spec00_crafty.sh


if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

if [ ! -e ${BENCH_REPO}/scripts/${BENCH_SCRIPT} ]; then
  echo "ERROR: '${BENCH_REPO}/scripts/${BENCH_SCRIPT}' not found"
  exit -1
fi


if [ ! -e ${ESESC_BIN} ]; then
  echo "BIN is: '$ESESC_BIN'"
  $ESESC_SRC/conf/scripts/build-with-docker.sh $ESESC_SRC $ESESC_BUILD_DIR $BUILD_TYPE
fi

docker run  \
  -v $ESESC_SRC:$ESESC_SRC \
  -v $ESESC_BUILD_DIR:$ESESC_BUILD_DIR \
  -v $BENCH_REPO:$BENCH_REPO:ro \
  --interactive=true \
  --tty=true \
  --privileged=true \
  ${DOCKER_IMAGE} \
  bin/bash 

