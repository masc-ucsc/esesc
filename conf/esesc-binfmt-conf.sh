#!/bin/sh
# enable automatic ARM/SPARC program execution by the kernel
# This code is based on qemu-binfmt-conf.sh from qemu

# load the binfmt_misc module
if [ ! -d /proc/sys/fs/binfmt_misc ]; then
  /sbin/modprobe binfmt_misc
fi
if [ ! -f /proc/sys/fs/binfmt_misc/register ]; then
  mount binfmt_misc -t binfmt_misc /proc/sys/fs/binfmt_misc
fi

echo "Do not run this script on an ARM CPU or you will have to reboot!!"

echo   ':arm:M::\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x28\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/local/esesc/esescbinfmt-arm:' > /proc/sys/fs/binfmt_misc/register
echo   ':sparc:M::\x7fELF\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x02:\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff:/usr/local/esesc/esescbinfmt-sparc:' > /proc/sys/fs/binfmt_misc/register

