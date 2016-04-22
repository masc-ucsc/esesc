#!/bin/bash
PREFIX=$HOME

$PREFIX/projs/esesc/conf/scripts/run-bench-with-docker.sh \
  $PREFIX/build/docker-esesc/debug/main/ \
  $PREFIX/runs \
  $1 \
  $PREFIX/projs/benchmarks \
  $PREFIX/projs/esesc/conf



