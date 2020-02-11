PROJECT(esesc)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")
SET(CMAKE_MODULE_PATH "${esesc_SOURCE_DIR}/conf/cmake-modules")
cmake_minimum_required(VERSION 2.7)

IF(ESESC_SYSTEM)
  MESSAGE("  -DESESC_SYSTEM=1              Enable esesc/qemu full system simulation")
ELSE(ESESC_SYSTEM)
  MESSAGE("  -DESESC_SYSTEM=0              Enable esesc/qemu user mode simulation")
ENDIF(ESESC_SYSTEM)

IF(ESESC_MIPSR6)
  MESSAGE("  -DESESC_MIPSR6=1              MIPSR6 QEMU")
ELSE(ESESC_MIPSR6)
  MESSAGE("  -DESESC_MIPSR6=0              RISCV QEMU")
ENDIF(ESESC_MIPSR6)

IF(ESESC_TRACE_DATA)
  MESSAGE("  -DESESC_TRACE_DATA=1          Get qemu addr and data for dinst use")
ENDIF(ESESC_TRACE_DATA)


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

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
  SET(QEMU_EXTRA --enable-debug --cc=${CMAKE_C_COMPILER})
ELSE(CMAKE_BUILD_TYPE MATCHES "Debug")
  SET(QEMU_EXTRA --cc=${CMAKE_C_COMPILER})
ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")

IF(ESESC_MIPSR6)
  IF(ESESC_SYSTEM)
    SET(QEMU_TARGET --target-list=mips64el-softmmu)
  ELSE(ESESC_SYSTEM)
    SET(QEMU_TARGET --target-list=mips64el-linux-user)
  ENDIF(ESESC_SYSTEM)
ELSE(ESESC_MIPSR6)
  IF(ESESC_SYSTEM)
    SET(QEMU_TARGET --target-list=riscv64-softmmu)
  ELSE(ESESC_SYSTEM)
    SET(QEMU_TARGET --target-list=riscv64-linux-user)
  ENDIF(ESESC_SYSTEM)
ENDIF(ESESC_MIPSR6)

ExternalProject_Add(qemu
  SOURCE_DIR ${qemu_SOURCE}
  CONFIGURE_COMMAND ${qemu_SOURCE}/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/qemu ${QEMU_TARGET}
  --disable-virtfs --disable-numa --disable-nettle --disable-gcrypt --disable-bzip2 --disable-libssh2 --disable-curl
  --disable-linux-aio --disable-sdl --disable-docs --enable-esesc --disable-capstone --python=python2.7 --disable-seccomp --disable-spice --disable-tools --disable-werror --disable-guest-agent --disable-gtk --disable-libusb --disable-libnfs --disable-xen --disable-snappy --disable-lzo --disable-gnutls --disable-vnc --disable-bluez ${QEMU_EXTRA}
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


