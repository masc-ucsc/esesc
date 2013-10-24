project(libpanal)

SET(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG=1 -Wall")
SET(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG=1 -Wall")

INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})

FILE(GLOB panal_SOURCE *.cpp *.c)
FILE(GLOB panal_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${panal_SOURCE})
SOURCE_GROUP("Header Files" FILES ${panal_HEADER})

FILE(GLOB exec_SOURCE1 panalyzer.cpp)

LIST(REMOVE_ITEM panal_SOURCE ${exec_SOURCE1})

ADD_LIBRARY(panal ${panal_SOURCE} ${panal_HEADER})

##########################
# panal

ADD_EXECUTABLE(panalyzer EXCLUDE_FROM_ALL ${exec_SOURCE1})

TARGET_LINK_LIBRARIES("panalyzer" panal suc ${ZLIB_LIBRARIES}) 
