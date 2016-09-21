PROJECT(esesc) 
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")
SET(CMAKE_MODULE_PATH "${esesc_SOURCE_DIR}/conf/cmake-modules")
cmake_minimum_required(VERSION 2.7)

#IF(ENABLE_PEQ)
#  MESSAGE("  -DENABLE_PEQ=1                Enable Equation based power model support")
#ELSE(ENABLE_PEQ)
#  MESSAGE("  -DENABLE_PEQ=0                Disable Equation based power model Fusupport (default)")
#ENDIF(ENABLE_PEQ)

IF(ESESC_SYSTEM)
  MESSAGE("  -DESESC_SYSTEM=1              Enable esesc/qemu full system simulation")
ELSE(ESESC_SYSTEM)
  MESSAGE("  -DESESC_SYSTEM=0              Enable esesc/qemu user mode simulation")
ENDIF(ESESC_SYSTEM)
  
IF(ENABLE_LIVE)
  MESSAGE("  -DESESC_LIVE=1                Enable esesc live sampling support")
ELSE(ENABLE_LIVE)
  MESSAGE("  -DESESC_LIVE=0                Disable esesc live compilation (default)")
ENDIF(ENABLE_LIVE)

IF(ENABLE_LIVECRIU)
  MESSAGE("  -DESESC_LIVECRIU=1                Enable CRIU checkpoints with live sampling support")
ELSE(ENABLE_LIVECRIU)
  MESSAGE("  -DESESC_LIVECRIU=0                Disable CRIU checkpoints with live compilation (default)")
ENDIF(ENABLE_LIVECRIU)


#############
MESSAGE("  -DCMAKE_HOST_MARCH=${CMAKE_HOST_MARCH} compilation")

#############
# Debug vs Release
IF(CMAKE_BUILD_TYPE MATCHES "Debug")
  MESSAGE("  -DCMAKE_BUILD_TYPE=Debug      debug compilation options")
ELSE(CMAKE_BUILD_TYPE MATCHES "Debug")
  MESSAGE("  -DCMAKE_BUILD_TYPE=Release    release compilation options (default)")
ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")

################################################
# use cmake -DDEBUG=1 to activate debugging
FIND_PACKAGE(ZLIB)
FIND_PACKAGE(Threads)
FIND_PACKAGE(Curses)
FIND_PACKAGE(FLEX)
FIND_PACKAGE(BISON)
FIND_PACKAGE(X11)
FIND_PACKAGE(UUID)
FIND_PACKAGE(Pixman)
FIND_PACKAGE(Pango)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# OS X Does not need SDL to run QEMU
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  INCLUDE(FindPkgConfig)
  PKG_SEARCH_MODULE(SDL2_REQUIRED sdl2)
  FIND_LIBRARY(RT_LIBRARIES NAMES rt)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

FIND_LIBRARY(MLIB_LIBRARIES NAMES m)
FIND_LIBRARY(UTIL_LIBRARIES NAMES util)

#################################
ADD_SUBDIRECTORY(misc/libsuc)

include(ExternalProject)

ExternalProject_Add(regression
  SOURCE_DIR ${esesc_SOURCE_DIR}/gold
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/gold/configure ${esesc_SOURCE_DIR}/gold ${CMAKE_CURRENT_BINARY_DIR}/gold
  PREFIX ${esesc_SOURCE_DIR}/gold
  BUILD_COMMAND sh gen-regression.sh ${BENCHSUITS_DIR}
  BUILD_IN_SOURCE "" 
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold/stampdir
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  )
set_target_properties(regression PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(gold
  SOURCE_DIR ${esesc_SOURCE_DIR}/gold
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/gold/configure ${esesc_SOURCE_DIR}/gold ${CMAKE_CURRENT_BINARY_DIR}/gold
  PREFIX ${esesc_SOURCE_DIR}/gold
  BUILD_COMMAND sh gen-gold.sh ${BENCHSUITS_DIR}
  BUILD_IN_SOURCE "" 
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold/stampdir
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  )
set_target_properties(gold PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(floorplan
  SOURCE_DIR ${esesc_SOURCE_DIR}/pwth/floorplan
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/pwth/floorplan/configure ${esesc_SOURCE_DIR}/pwth/floorplan ${CMAKE_CURRENT_BINARY_DIR}/floorplan
  PREFIX ${esesc_SOURCE_DIR}/pwth/floorplan
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/floorplan
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/floorplan/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/floorplan/stampdir
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  )
set_target_properties(floorplan PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(bfd
  SOURCE_DIR ${esesc_SOURCE_DIR}/misc/bfd
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/misc/bfd/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/bfd CFLAGS=${CMAKE_C_FLAGS}\ -g\ -I${esesc_SOURCE_DIR}/misc/include --enable-64-bit-bfd
  PREFIX ${esesc_SOURCE_DIR}/misc/bfd
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bfd
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/bfd/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/bfd/stampdir
  DOWNLOAD_COMMAND ""
  )
set_target_properties(bfd PROPERTIES EXCLUDE_FROM_ALL TRUE)
set(bfd_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/bfd/libbfd.a)

ExternalProject_Add(opcodes
  SOURCE_DIR ${esesc_SOURCE_DIR}/misc/opcodes
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/misc/opcodes/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/opcodes CFLAGS=${CMAKE_C_FLAGS}\ -I${esesc_SOURCE_DIR}/misc/include
  PREFIX ${esesc_SOURCE_DIR}/misc/opcodes
  DEPENDS bfd
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/opcodes
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/opcodes/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/opcodes/stampdir
  DOWNLOAD_COMMAND ""
  )
set_target_properties(opcodes PROPERTIES EXCLUDE_FROM_ALL TRUE)
set(opcodes_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/opcodes/libopcodes.a)

ExternalProject_Add(libiberty
  SOURCE_DIR ${esesc_SOURCE_DIR}/misc/libiberty
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/misc/libiberty/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/libiberty CFLAGS=${CMAKE_C_FLAGS}\ -I${esesc_SOURCE_DIR}/misc/include
  PREFIX ${esesc_SOURCE_DIR}/misc/libiberty
  DEPENDS bfd
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libiberty
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/libiberty/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/libiberty/stampdir
  DOWNLOAD_COMMAND ""
  )
set_target_properties(libiberty PROPERTIES EXCLUDE_FROM_ALL TRUE)
set(libiberty_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libiberty/libiberty.a)

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
  SET(QEMU_EXTRA --enable-debug --cc=${CMAKE_C_COMPILER})
ELSE(CMAKE_BUILD_TYPE MATCHES "Debug")
  SET(QEMU_EXTRA --cc=${CMAKE_C_COMPILER})
ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")

IF(ESESC_SYSTEM)
  SET(QEMU_TARGET --target-list=mips64el-softmmu)
ELSE(ESESC_SYSTEM)
  IF(ESESC_N32)
    SET(QEMU_TARGET --target-list=mipsn32el-linux-user)
  ELSE(ESESC_N32)
    SET(QEMU_TARGET --target-list=mips64el-linux-user)
  ENDIF(ESESC_N32)
ENDIF(ESESC_SYSTEM)

ExternalProject_Add(qemu
  SOURCE_DIR ${esesc_SOURCE_DIR}/emul/qemu
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/emul/qemu/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/qemu ${QEMU_TARGET} --disable-virtfs --disable-numa --disable-nettle --disable-gcrypt --disable-bzip2 --disable-libssh2 --disable-curl --disable-linux-aio --disable-vhdx --disable-sdl --disable-docs --enable-esesc --python=python2 --disable-seccomp --disable-spice --disable-tools --disable-werror --disable-guest-agent --disable-gtk --disable-libusb --disable-libnfs --disable-xen --disable-snappy --disable-lzo --disable-smartcard --disable-gnutls --disable-vnc --disable-bluez ${QEMU_EXTRA}
  PREFIX ${esesc_SOURCE_DIR}/emul/qemu
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/qemu
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/qemu/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/qemu/stampdir
  DOWNLOAD_COMMAND ""
  )
set_target_properties(qemu PROPERTIES EXCLUDE_FROM_ALL TRUE)

#################################
#ADD_SUBDIRECTORY(misc/libmigdb)
ADD_SUBDIRECTORY(emul/libemulint)
ADD_SUBDIRECTORY(emul/libqemuint)
#################################
ADD_SUBDIRECTORY(pwth/libpeq)
ADD_SUBDIRECTORY(pwth/libmcpat)
ADD_SUBDIRECTORY(pwth/libsesctherm)
ADD_SUBDIRECTORY(pwth/libpwrmodel)
#################################
ADD_SUBDIRECTORY(simu/libcore)
ADD_SUBDIRECTORY(simu/libsampler)
ADD_SUBDIRECTORY(simu/libmem)
ADD_SUBDIRECTORY(simu/libnet)
#################################
ADD_SUBDIRECTORY(main)
ADD_SUBDIRECTORY(tests/gtest-1.7.0)
ADD_SUBDIRECTORY(tests)


