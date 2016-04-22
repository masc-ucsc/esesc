#!/bin/bash

if [ "$#" -lt 2 ]; then
  echo "Usage: <esesc_src_dir> <build_dir> [build_type] [enable_live]"
  exit -1
fi

ESESC_SRC=$1
HOST_BUILD_DIR=$2
BUILD_TYPE=${3:-Debug}
ENABLE_LIVE=${4:-1}

: ${ESESC_HOST_PROCS:=`nproc`}


if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "BUILD ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi


# Note: the 'chown' command is used to set permissions
# on the generated files to the current user rather than root.  There
# is not an option in Docker yet to do this automatically, but there is
# an open issue in GitHub: https://github.com/docker/docker/issues/7198
# so there may be a better way in the future 

ESESC_SRC=${ESESC_SRC} \
ESESC_BUILD_DIR=${HOST_BUILD_DIR} \
ESESC_BUILD_TYPE=${BUILD_TYPE} \
ESESC_HOST_PROCS=${ESESC_HOST_PROCS} \
ESESC_ENABLE_LIVE=${ENABLE_LIVE} \
ESESC_ENABLE_LIVECRIU=${ENABLE_LIVE} \
${ESESC_SRC}/conf/scripts/build.sh

