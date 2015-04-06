project(suc)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

FILE(GLOB suc_SOURCE *.cpp)
FILE(GLOB suc_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${suc_SOURCE})
SOURCE_GROUP("Header Files" FILES ${suc_HEADER})

FILE(GLOB exec_SOURCE1 poolBench.cpp)

LIST(REMOVE_ITEM suc_SOURCE ${exec_SOURCE1})

ADD_LIBRARY(suc ${suc_SOURCE} ${PROJECT_BINARY_DIR}/confparser.cpp ${PROJECT_BINARY_DIR}/conflexer.cpp ${suc_HEADER})

FILE(GLOB flex_SOURCE conflex.l)
FILE(GLOB bison_SOURCE conflex.y)

BISON_TARGET(confparser ${bison_SOURCE} ${PROJECT_BINARY_DIR}/confparser.cpp COMPILE_FLAGS "-p yyConf")
FLEX_TARGET(conflexer   ${flex_SOURCE} ${PROJECT_BINARY_DIR}/conflexer.cpp COMPILE_FLAGS "-Cemr")
#ADD_FLEX_BISON_DEPENDENCY(conflexer confparser)

INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR} ${suc_SOURCE_DIR})

##########################
# poolBench

ADD_EXECUTABLE(poolBench EXCLUDE_FROM_ALL ${exec_SOURCE1})

TARGET_LINK_LIBRARIES("poolBench" suc -lpthread)

 