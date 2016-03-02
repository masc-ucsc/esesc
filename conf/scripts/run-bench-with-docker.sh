#!/bin/bash

if [ "$#" -lt 3 ]; then
  echo "Usage: <esesc_bin_dir> <run_dir> <benchmark_script> [benchmarks_repo] [esesc_conf_dir] [docker_image]"
  exit -1
fi

ESESC_BIN_DIR=$1
RUN_DIR=$2
BENCH_SCRIPT=$3
BENCH_REPO=${4:-${HOME}/projs/benchmarks}
CONF_DIR=${5:-${HOME}/projs/esesc/conf}
DOCKER_IMAGE=${6:-mascucsc/esescbase}

ESESC_BIN=${ESESC_BIN_DIR}/esesc

if [ ! -e ${ESESC_BIN} ]; then
  echo "ERROR: '${ESESC_BIN}' does not exist"
  exit -1
fi

if [ ! -e ${BENCH_REPO}/scripts/${BENCH_SCRIPT} ]; then
  echo "ERROR: '${BENCH_REPO}/scripts/${BENCH_SCRIPT}' not found"
  exit -1
fi

if [ ! -e ${CONF_DIR}/esesc.conf ]; then
  echo "ERROR: '${CONF_DIR}/esesc.conf' not found"
  exit -1
fi

GID=`id -rg`

# Note: the 'chown' command is used to set permissions
# on the generated files to the current user rather than root.  There
# is not an option in Docker yet to do this automatically, but there is
# an open issue in GitHub: https://github.com/docker/docker/issues/7198
# so there may be a better way in the future 
docker pull $DOCKER_IMAGE

# Keep local version of run directory 
#  -v $RUN_DIR:$RUN_DIR \

docker run \
  --detach \
  --net=host \
  -v $ESESC_BIN_DIR:$ESESC_BIN_DIR:ro \
  -v $BENCH_REPO:$BENCH_REPO:ro \
  -v $CONF_DIR:$CONF_DIR:ro \
  ${DOCKER_IMAGE} \
  bin/bash -c " \
  mkdir -p ${RUN_DIR} && \
  cd ${RUN_DIR} && \
  cp ${CONF_DIR}/*conf* . && \
  ln -s ${BENCH_REPO}/bins bins && \
  ln -s ${BENCH_REPO}/scripts scripts && \
  ln -s ${BENCH_REPO}/data data && \
  scripts/${BENCH_SCRIPT} ${ESESC_BIN} && \
  unlink bins && unlink data && unlink scripts && \
  chown -R ${UID}:${GID} ${RUN_DIR}"

