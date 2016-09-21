#!/bin/bash

ESESC_SRC=$1

${ESESC_SRC}/emul/qemu/configure --prefix=/local/gsouther/build/esesc_test/qemu --target-list=mips64el-linux-user --disable-docs  --python=python2 --disable-seccomp --disable-spice --disable-tools --disable-werror --disable-guest-agent --disable-gtk --disable-libusb --disable-libnfs --disable-xen --disable-snappy --disable-lzo --disable-smartcard-nss --disable-gnutls --disable-vnc --disable-bluez --cc=/usr/bin/cc

