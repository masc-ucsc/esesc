project(pwrmodel)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
INCLUDE_DIRECTORIES(${mcpat_SOURCE_DIR})
INCLUDE_DIRECTORIES(${sesctherm_SOURCE_DIR})
#IF(ENABLE_PEQ)
INCLUDE_DIRECTORIES(${peq_SOURCE_DIR})
#ENDIF(ENABLE_PEQ)

FILE(GLOB pwrmodel_SOURCE *.cpp)
FILE(GLOB pwrmodel_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${pwrmodel_SOURCE})
SOURCE_GROUP("Header Files" FILES ${pwrmodel_HEADER})

FILE(GLOB exec_SOURCE2 wrapper_ut.cpp)

LIST(REMOVE_ITEM pwrmodel_SOURCE ${exec_SOURCE2})

ADD_LIBRARY(pwrmodel ${pwrmodel_SOURCE} ${pwrmodel_HEADER})

ADD_EXECUTABLE(wrapper_ut EXCLUDE_FROM_ALL ${exec_SOURCE2})

TARGET_LINK_LIBRARIES("wrapper_ut" pwrmodel mcpat suc -lpthread)
