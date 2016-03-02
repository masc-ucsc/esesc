#!/bin/bash

PREFIX=$HOME

CKPT_IMG_NUM=$2
CKPT_IMG_DIR=/local/gsouther/runs/checkpoints/cp${CKPT_IMG_NUM}

IMG_HASH=$1


ESESC_BIN_DIR=$PREFIX/build/docker-esesc/debug/main
BENCH_REPO=$PREFIX/projs/benchmarks
CONF_DIR=$PREFIX/projs/esesc/conf

CONTAINER=$(docker create \
  -v $ESESC_BIN_DIR:$ESESC_BIN_DIR:ro \
  -v $BENCH_REPO:$BENCH_REPO:ro \
  -v $CONF_DIR:$CONF_DIR:ro \
  ${IMG_HASH})

docker restore --force --image-dir $CKPT_IMG_DIR $CONTAINER
