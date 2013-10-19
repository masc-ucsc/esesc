PROJECT(softfloat)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

IF(ESESC_QEMU_ISA_ARMEL)
  #  ADD_SUBDIRECTORY(littleendian)
  INCLUDE_DIRECTORIES(${softfloat_SOURCE_DIR}/littleendian)
  LINK_DIRECTORIES(${softfloat_BINARY_DIR}/littleendian)
ELSE(ESESC_QEMU_ISA_ARMEL)
  #  ADD_SUBDIRECTORY(bigendian)
  INCLUDE_DIRECTORIES(${softfloat_SOURCE_DIR}/bigeendian)
  LINK_DIRECTORIES(${softfloat_BINARY_DIR}/bigeendian)
ENDIF(ESESC_QEMU_ISA_ARMEL)


SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${softfloat_SOURCE_DIR}/littleendian")

FILE(GLOB softfloat_SOURCE *.cpp)
FILE(GLOB softfloat_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${softfloat_SOURCE})
SOURCE_GROUP("Header Files" FILES ${softfloat_HEADER})

ADD_LIBRARY(softfloat ${softfloat_SOURCE} ${softfloat_HEADER})

#ADD_EXECUTABLE(softfloatmain softfloatmain.cpp)
#TARGET_LINK_LIBRARIES("softfloatmain"  softfloat -Wl,${qemu_SOURCE_DIR}/arm-linux-user/libqemu_esesc.a -lpthread -lz -lncurses -lSM -lICE -lX11 -lXext -lm -lrt -lutil)
