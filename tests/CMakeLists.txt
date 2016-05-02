project(dgraph)
cmake_minimum_required(VERSION 2.7)

enable_testing()
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")
INCLUDE_DIRECTORIES(${suc_SOURCE_DIR})
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
INCLUDE_DIRECTORIES(${gtest_SOURCE_DIR}/include)
link_directories(${gtest_BINARY_DIR}/src)

SET(ALLTESTS cachetest bwtest)
FOREACH(TEST ${ALLTESTS})
  add_executable(${TEST} ${TEST}.cpp)
  target_link_libraries(${TEST} gtest_main sampler mem core pwrmodel mcpat sesctherm peq emulint suc gtest) 

  add_test(NAME ${TEST} COMMAND ${TEST})

  set_target_properties(${TEST} PROPERTIES DEPENDS sampler)
  set_target_properties(${TEST} PROPERTIES DEPENDS mem)

  set_target_properties(${TEST} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	ADD_CUSTOM_COMMAND (TARGET ${TEST}
	POST_BUILD
	COMMAND    "cp"
	ARGS       ${esesc_SOURCE_DIR}/conf/*.conf .
	)

ENDFOREACH(TEST)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS ${ALLTESTS})
set_target_properties(check PROPERTIES EXCLUDE_FROM_ALL TRUE)


