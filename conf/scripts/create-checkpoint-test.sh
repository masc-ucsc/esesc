#!/bin/bash

CONTAINER=$1
CKPT_NUM=$2

CKPT=$(docker commit $1)
docker checkpoint --leave-running --image-dir=/local/gsouther/runs/checkpoints/cp${CKPT_NUM} ${CONTAINER}


