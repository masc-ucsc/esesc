#!/bin/bash

if [ "$#" -lt 2 ]; then
  echo "Usage: <esesc_src_dir> <build_dir> [build_type] [enable_live] [docker_image]"
  exit -1
fi

ESESC_SRC=$1
HOST_BUILD_DIR=$2
BUILD_TYPE=${3:-Debug}
ENABLE_LIVE=${4:-1}
DOCKER_IMAGE=${5:-mascucsc/esescbase}

: ${ESESC_HOST_PROCS:=`nproc`}

DOCKER_ESESC_SRC='/esesc'
DOCKER_BUILD_DIR='/build'

if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "BUILD ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

GID=`id -rg`

# Note: the 'chown' command is used to set permissions
# on the generated files to the current user rather than root.  There
# is not an option in Docker yet to do this automatically, but there is
# an open issue in GitHub: https://github.com/docker/docker/issues/7198
# so there may be a better way in the future 

docker run  -t \
  -v $ESESC_SRC:$DOCKER_ESESC_SRC \
  -v $HOST_BUILD_DIR:$DOCKER_BUILD_DIR \
  -e ESESC_SRC=${DOCKER_ESESC_SRC} \
  -e ESESC_BUILD_DIR=${DOCKER_BUILD_DIR} \
  -e ESESC_BUILD_TYPE=${BUILD_TYPE} \
  -e ESESC_HOST_PROCS=${ESESC_HOST_PROCS} \
  -e ESESC_ENABLE_LIVE=${ENABLE_LIVE} \
  -e ESESC_ENABLE_LIVECRIU=${ENABLE_LIVE} \
  ${DOCKER_IMAGE} \
  bin/bash -c " \
  /esesc/conf/scripts/build.sh && \
  chown -R ${UID}:${GID} ${DOCKER_BUILD_DIR}"

