project(migdb)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

FILE(GLOB migdb_SOURCE *.cpp)
FILE(GLOB migdb_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${migdb_SOURCE})
SOURCE_GROUP("Header Files" FILES ${migdb_HEADER})

#FILE(GLOB exec_SOURCE1 poolBench.cpp)

#LIST(REMOVE_ITEM suc_SOURCE ${exec_SOURCE1})

ADD_LIBRARY(migdb ${migdb_SOURCE} ${migdb_HEADER})

