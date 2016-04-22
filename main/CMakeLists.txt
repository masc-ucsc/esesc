project(main)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
INCLUDE_DIRECTORIES(${emulint_SOURCE_DIR})
INCLUDE_DIRECTORIES(${core_SOURCE_DIR})
INCLUDE_DIRECTORIES(${mem_SOURCE_DIR})

INCLUDE_DIRECTORIES(${net_SOURCE_DIR})
INCLUDE_DIRECTORIES(${rst_SOURCE_DIR}) 
 
INCLUDE_DIRECTORIES(${mcpat_SOURCE_DIR}) 
INCLUDE_DIRECTORIES(${peq_SOURCE_DIR}) 
INCLUDE_DIRECTORIES(${pwrmodel_SOURCE_DIR})
INCLUDE_DIRECTORIES(${sesctherm_SOURCE_DIR})
INCLUDE_DIRECTORIES(${sampler_SOURCE_DIR})

INCLUDE_DIRECTORIES(${qemuint_SOURCE_DIR}) 
INCLUDE_DIRECTORIES(${gpuint_SOURCE_DIR}) 
INCLUDE_DIRECTORIES(${scqemu_SOURCE_DIR}) 
INCLUDE_DIRECTORIES(${migdb_SOURCE_DIR}) 

FILE(GLOB main_SOURCE *.cpp)
FILE(GLOB main_HEADER *.h)

SOURCE_GROUP("Source Files" FILES ${main_SOURCE})
SOURCE_GROUP("Header Files" FILES ${main_HEADER})

##########################
# esesc and mainbench

SET(EXELIST "esesc" "lsqtest" "qemumain" "qemumin" "membench" "netBench" "cachebench")

FOREACH(EXE ${EXELIST})
	FILE(GLOB exec_SOURCE "${EXE}.cpp")
  IF(EXE MATCHES "esesc")
	  ADD_EXECUTABLE(${EXE} ${exec_SOURCE})
  ELSE(EXE MATCHES "esesc")
	  ADD_EXECUTABLE(${EXE} EXCLUDE_FROM_ALL  ${exec_SOURCE})
  ENDIF(EXE MATCHES "esesc")
	LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
ENDFOREACH(EXE)

## live is special because it does not use esesc libraries
FILE(GLOB exec_SOURCE "live.cpp")
FILE(GLOB rabbitso_SOURCE "rabbitso.cpp")
FILE(GLOB esescso_SOURCE "esescso.cpp")
ADD_EXECUTABLE(live EXCLUDE_FROM_ALL ${exec_SOURCE} "../misc/libsuc/Transporter.cpp" "../misc/libsuc/LiveCache.cpp" "../misc/libsuc/LiveCacheCore.cpp" "../misc/libsuc/nanassert.cpp" "../misc/libsuc/Snippets.cpp")
LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
#ADD_EXECUTABLE(live EXCLUDE_FROM_ALL ${exec_SOURCE})

## live is special because it does not use esesc libraries
FILE(GLOB exec_SOURCE "livecriu.cpp")
ADD_EXECUTABLE(livecriu EXCLUDE_FROM_ALL ${exec_SOURCE} "../misc/libsuc/Transporter.cpp" "../misc/libsuc/LiveCache.cpp" "../misc/libsuc/LiveCacheCore.cpp" "../misc/libsuc/nanassert.cpp" "../misc/libsuc/Snippets.cpp")

LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
TARGET_LINK_LIBRARIES(live ${CMAKE_QEMU_OBJS})
TARGET_LINK_LIBRARIES(live emulint ${CMAKE_THREAD_LIBS_INIT} -lrt)
TARGET_LINK_LIBRARIES(live ${CMAKE_QEMU_LIBS})
TARGET_LINK_LIBRARIES(livecriu ${CMAKE_QEMU_OBJS})
TARGET_LINK_LIBRARIES(livecriu emulint ${CMAKE_THREAD_LIBS_INIT} -lrt)
TARGET_LINK_LIBRARIES(livecriu ${CMAKE_QEMU_LIBS})
LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
LIST(REMOVE_ITEM main_SOURCE ${rabbitso_SOURCE})
LIST(REMOVE_ITEM main_SOURCE ${esescso_SOURCE})
ADD_LIBRARY(esescso SHARED ${esescso_SOURCE} ${main_HEADER})
TARGET_LINK_LIBRARIES(esescso main sampler mem core pwrmodel mcpat sesctherm peq qemuint emulint suc)
ADD_LIBRARY(rabbitso SHARED ${rabbitso_SOURCE} ${main_HEADER} "../misc/libsuc/LiveCache.cpp" "../misc/libsuc/LiveCacheCore.cpp" "../misc/libsuc/nanassert.cpp" "../misc/libsuc/Snippets.cpp")
add_dependencies(live rabbitso)
add_dependencies(live esescso)
target_link_libraries(live ${CMAKE_DL_LIBS})

add_dependencies(livecriu rabbitso)
add_dependencies(livecriu esescso)
target_link_libraries(livecriu ${CMAKE_DL_LIBS})


###############################
ADD_LIBRARY(main ${main_SOURCE} ${main_HEADER})

FOREACH(EXE ${EXELIST})
  add_dependencies(${EXE} qemu)
  TARGET_LINK_LIBRARIES(${EXE} ${CMAKE_QEMU_OBJS})
  TARGET_LINK_LIBRARIES(${EXE} main sampler mem net core pwrmodel mcpat sesctherm peq)
  TARGET_LINK_LIBRARIES(${EXE} qemuint emulint suc)
  TARGET_LINK_LIBRARIES(${EXE} ${CMAKE_QEMU_LIBS})
ENDFOREACH(EXE)

