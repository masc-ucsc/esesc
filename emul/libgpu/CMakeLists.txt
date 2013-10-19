project(gpuint)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
INCLUDE_DIRECTORIES(${crack_SOURCE_DIR})
INCLUDE_DIRECTORIES(${emulint_SOURCE_DIR})
INCLUDE_DIRECTORIES(${qemuint_SOURCE_DIR})
INCLUDE_DIRECTORIES(${core_SOURCE_DIR})
#INCLUDE_DIRECTORIES("../libqemuint")

FILE(GLOB gpuint_SOURCE *.cpp)
FILE(GLOB gpuint_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${gpuint_SOURCE})
SOURCE_GROUP("Header Files" FILES ${gpuint_HEADER})

ADD_LIBRARY(gpuint ${gpuint_SOURCE} ${gpuint_HEADER})

##########################
# gpumain

#ADD_EXECUTABLE(gpumain ${exec_SOURCE1})

#TARGET_LINK_LIBRARIES("gpumain" gpuint qemuint emulint core crack suc)
#TARGET_LINK_LIBRARIES("gpumain" ${CMAKE_QEMU_LIBS})
