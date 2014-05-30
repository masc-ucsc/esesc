cmake_minimum_required(VERSION 2.7)
project(unittest)

INCLUDE("${esesc_SOURCE_DIR}/CMake.common")

INCLUDE_DIRECTORIES(${main_SOURCE_DIR})
INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
INCLUDE_DIRECTORIES(${crack_SOURCE_DIR})
INCLUDE_DIRECTORIES(${mem_SOURCE_DIR})
INCLUDE_DIRECTORIES(${core_SOURCE_DIR})
INCLUDE_DIRECTORIES(${emulint_SOURCE_DIR})

INCLUDE_DIRECTORIES(${rst_SOURCE_DIR}) 

INCLUDE_DIRECTORIES(${mcpat_SOURCE_DIR}) 
INCLUDE_DIRECTORIES(${peq_SOURCE_DIR}) 
INCLUDE_DIRECTORIES(${pwrmodel_SOURCE_DIR})
INCLUDE_DIRECTORIES(${sesctherm_SOURCE_DIR})
INCLUDE_DIRECTORIES(${sampler_SOURCE_DIR})

INCLUDE_DIRECTORIES(${scqemu_SOURCE_DIR}) 
INCLUDE_DIRECTORIES(${migdb_SOURCE_DIR}) 

IF(ENABLE_QEMU_SYSTEM OR ENABLE_QEMU_USER) 
  INCLUDE_DIRECTORIES(${qemuint_SOURCE_DIR}) 
ENDIF(ENABLE_QEMU_SYSTEM OR ENABLE_QEMU_USER) 
IF(ENABLE_CUDA)
  INCLUDE_DIRECTORIES(${gpuint_SOURCE_DIR}) 
ENDIF(ENABLE_CUDA)


################################
# GTest
################################
ADD_SUBDIRECTORY (gtest-1.7.0)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

################################
# Unit Tests
################################
# Executable list of tests
SET(EXELIST "OoOProcessor_unittest" ) # "BPred_unittest" )

###############################
# Lists of libs for each test

# OoOProcessor_unittest libraries
# Order is IMPORTANT. Won't link if this is wrong
SET("OoOProcessor_unittest_LIBS" gtest gtest_main main mem sampler core pwrmodel mcpat sesctherm peq)
IF(ENABLE_CUDA)
    LIST(APPEND "OoOProcessor_unittest_LIBS"  gpuint)
ENDIF(ENABLE_CUDA)
IF(ENABLE_NOEMU)
    LIST(APPEND "OoOProcessor_unittest_LIBS" emulint crack suc ${CMAKE_THREAD_LIBS_INIT} -lrt)
ELSE(ENABLE_NOEMU)
    LIST(APPEND "OoOProcessor_unittest_LIBS" qemuint emulint crack suc)
    LIST(APPEND "OoOProcessor_unittest_LIBS" ${CMAKE_QEMU_LIBS})
ENDIF(ENABLE_NOEMU)

MESSAGE("Here: ${OoOProcessor_unittest_LIBS}")

# BPred_unittest libraries

###################################
# Add source files and create exes
FOREACH(EXE ${EXELIST})
    #MESSAGE("Building ${EXE} qemu")
	FILE(GLOB exec_SOURCE "${EXE}.cpp")
    ADD_EXECUTABLE( ${EXE} ${exec_SOURCE} ) # OoOProcessor_unittest OoOProcessor_unittest.cpp )
    #ADD_DEPENDENCIES(${EXE} suc crack mem core emulint qemuint sampler)
    # Remove from source files to prevent duplicates
    IF(NOT ENABLE_NOEMU)
        ADD_DEPENDENCIES(${EXE} qemu)
    ENDIF(NOT ENABLE_NOEMU)
ENDFOREACH(EXE)


# Link libraries for each exe
FOREACH(EXE ${EXELIST})
    #MESSAGE("Linking ${EXE} with: ${${EXE}_LIBS}")
    TARGET_LINK_LIBRARIES(${EXE} ${${EXE}_LIBS})
ENDFOREACH(EXE)

##################################
# Add all the unit tests and set to execute post-build

# Custom target for running unit tests
add_custom_target(test)
macro(RUN_TEST test_target)
  add_custom_target(${test_target}_runtest
      COMMAND ${test_target} #cmake 2.6 required
      DEPENDS ${test_target}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
  add_dependencies(test ${test_target}_runtest)
endmacro()

# Adding and running
FOREACH(EXE ${EXELIST})
    ADD_TEST( ${EXE} ${EXE})
    RUN_TEST(${EXE}) # can we remove this?
ENDFOREACH(EXE)

set(COPY_COUNTER 0)

# Macro for copying configuration and binary files to t folder
macro(copy_files GLOBPAT DESTINATION)
  file(GLOB COPY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLOBPAT})
  math(EXPR COPY_COUNTER "${COPY_COUNTER}+1")

  add_custom_target("copy${COPY_COUNTER}" ALL
    COMMENT "Copying files: ${GLOBPAT}") # to ${DESTINATION}, ${COPY_FILES}")

  foreach(FILENAME ${COPY_FILES})
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
    get_filename_component(FILENAME_BASE ${FILENAME} NAME) 
    set(DST "${DESTINATION}/${FILENAME_BASE}")
    #MESSAGE("SRC=${SRC} , DST=${DST}")

    add_custom_command(
      TARGET "copy${COPY_COUNTER}"
      COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
      )
  endforeach(FILENAME)
endmacro(copy_files)

# Copy config and bin files over to test directory
copy_files( "../bins/*" ${CMAKE_CURRENT_BINARY_DIR} )
copy_files( "../conf/*" ${CMAKE_CURRENT_BINARY_DIR} )


MESSAGE("-- Initialized unit test framework.")

