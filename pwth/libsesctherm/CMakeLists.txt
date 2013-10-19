project(sesctherm)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
INCLUDE_DIRECTORIES(${mcpat_SOURCE_DIR}) 

FILE(GLOB sesctherm_SOURCE *.cpp)
FILE(GLOB sesctherm_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${sesctherm_SOURCE})
SOURCE_GROUP("Header Files" FILES ${sesctherm_HEADER})

FILE(GLOB exec_SOURCE1 thermmain.cpp)
FILE(GLOB exec_SOURCE2 sesctherm_ut.cpp)

LIST(REMOVE_ITEM sesctherm_SOURCE ${exec_SOURCE1})
LIST(REMOVE_ITEM sesctherm_SOURCE ${exec_SOURCE2})

ADD_LIBRARY(sesctherm EXCLUDE_FROM_ALL ${sesctherm_SOURCE} ${sesctherm_HEADER})

ADD_EXECUTABLE(thermmain EXCLUDE_FROM_ALL ${exec_SOURCE1})
ADD_EXECUTABLE(sesctherm_ut EXCLUDE_FROM_ALL ${exec_SOURCE2})
TARGET_LINK_LIBRARIES("thermmain" sesctherm suc pwrmodel)
TARGET_LINK_LIBRARIES("sesctherm_ut" sesctherm suc pwrmodel)
