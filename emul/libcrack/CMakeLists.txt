project(crack)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
#INCLUDE_DIRECTORIES(${scqemu_SOURCE_DIR})
#INCLUDE_DIRECTORIES(${crack_SOURCE_DIR})

FILE(GLOB crack_SOURCE *.cpp)
FILE(GLOB crack_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${crack_SOURCE})
SOURCE_GROUP("Header Files" FILES ${crack_HEADER})

ADD_LIBRARY(crack ${crack_SOURCE} ${crack_HEADER})
