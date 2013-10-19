project(emulint)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
INCLUDE_DIRECTORIES(${crack_SOURCE_DIR})
INCLUDE_DIRECTORIES(${emulint_SOURCE_DIR})

FILE(GLOB emulint_SOURCE *.cpp)
FILE(GLOB emulint_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${emulint_SOURCE})
SOURCE_GROUP("Header Files" FILES ${emulint_HEADER})

ADD_LIBRARY(emulint ${emulint_SOURCE} ${emulint_HEADER})
