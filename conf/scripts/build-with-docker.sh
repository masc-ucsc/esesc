#!/bin/bash

if [ "$#" -lt 1 ]; then
  echo "Usage: <esesc_src_dir> <benchmarks> [build_type] [docker_image]"
  exit -1
fi

TAG_DATE=${TAG_DATE:-$(date +'%Y.%m')}

ESESC_SRC=$1
BUILD_TYPE=${2:-Debug}
DOCKER_IMAGE=${3:-mascucsc/archlinux-masc:latest}

: ${ESESC_HOST_PROCS:=$(nproc)}

DOCKER_ESESC_SRC='/root/projs/esesc'
DOCKER_BUILD_DIR='/root/projs/build'

if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "BUILD ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

docker run  -t \
  -v $ESESC_SRC:$DOCKER_ESESC_SRC \
  -e ESESC_SRC=${DOCKER_ESESC_SRC} \
  -e ESESC_BUILD_DIR=${DOCKER_BUILD_DIR} \
  -e ESESC_BUILD_TYPE=${BUILD_TYPE} \
  -e ESESC_HOST_PROCS=${ESESC_HOST_PROCS} \
  ${DOCKER_IMAGE} /root/projs/esesc/conf/scripts/build.sh

