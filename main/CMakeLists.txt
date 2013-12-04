project(main)
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
INCLUDE_DIRECTORIES(${crack_SOURCE_DIR})
INCLUDE_DIRECTORIES(${emulint_SOURCE_DIR})
INCLUDE_DIRECTORIES(${core_SOURCE_DIR})
INCLUDE_DIRECTORIES(${mem_SOURCE_DIR})

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

IF(ENABLE_CUDA)
  SET(EXELIST "esesc" "lsqtest" "qemumain" "membench" "cachebench""gpumain")
ELSE(ENABLE_CUDA)
  SET(EXELIST "esesc" "lsqtest" "qemumain" "membench" "cachebench")
  FILE(GLOB exec_SOURCE "gpumain.cpp")
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
ENDIF(ENABLE_CUDA)

FOREACH(EXE ${EXELIST})
	FILE(GLOB exec_SOURCE "${EXE}.cpp")
  IF(EXE MATCHES "esesc")
	  ADD_EXECUTABLE(${EXE} ${exec_SOURCE})
  ELSE(EXE MATCHES "esesc")
	  ADD_EXECUTABLE(${EXE} EXCLUDE_FROM_ALL  ${exec_SOURCE})
  ENDIF(EXE MATCHES "esesc")
	LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
ENDFOREACH(EXE)

add_dependencies(esesc qemu)
add_dependencies(qemumain qemu)
add_dependencies(membench qemu)

## qemumin is special because it does not use esesc libraries
FILE(GLOB exec_SOURCE "qemumin.cpp")
ADD_EXECUTABLE(qemumin EXCLUDE_FROM_ALL ${exec_SOURCE})
LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
TARGET_LINK_LIBRARIES(qemumin ${CMAKE_QEMU_LIBS})

IF(ENABLE_SCQEMU)
  FILE(GLOB exec_SOURCE "scqemumain.cpp")
  ADD_EXECUTABLE(scqemumain  ${exec_SOURCE})
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})

  FILE(GLOB exec_SOURCE "generateARMTrace.cpp")
  ADD_EXECUTABLE(generateARMTrace  ${exec_SOURCE})
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})

  FILE(GLOB exec_SOURCE "scinst.cpp")
  ADD_EXECUTABLE(scinst  ${exec_SOURCE})
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})

  FILE(GLOB exec_SOURCE "scmain.cpp")
  ADD_EXECUTABLE(scmain  ${exec_SOURCE})
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})

  TARGET_LINK_LIBRARIES(scqemumain scqemu softfloat crack ${CMAKE_QEMU_LIBS})
  TARGET_LINK_LIBRARIES(generateARMTrace migdb)
  TARGET_LINK_LIBRARIES(scmain scqemu softfloat crack suc  ${bfd_LIBRARIES}  ${opcodes_LIBRARIES}  ${libiberty_LIBRARIES} -lz)
  TARGET_LINK_LIBRARIES(scinst softfloat crack suc)
  add_dependencies(scmain bfd opcodes libiberty)
ELSE(ENABLE_SCQEMU)
  FILE(GLOB exec_SOURCE "scqemumain.cpp")
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
  FILE(GLOB exec_SOURCE "scmain.cpp")
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
  FILE(GLOB exec_SOURCE "scinst.cpp")
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
  FILE(GLOB exec_SOURCE "scmain.cpp")
  LIST(REMOVE_ITEM main_SOURCE ${exec_SOURCE})
ENDIF(ENABLE_SCQEMU)

###############################
ADD_LIBRARY(main ${main_SOURCE} ${main_HEADER})

FOREACH(EXE ${EXELIST})
  TARGET_LINK_LIBRARIES(${EXE} main sampler mem core pwrmodel mcpat sesctherm peq)
  IF(ENABLE_CUDA)
    TARGET_LINK_LIBRARIES(${EXE} gpuint)
  ENDIF(ENABLE_CUDA)
  TARGET_LINK_LIBRARIES(${EXE} qemuint emulint crack suc)
  TARGET_LINK_LIBRARIES(${EXE} ${CMAKE_QEMU_LIBS})
ENDFOREACH(EXE)


