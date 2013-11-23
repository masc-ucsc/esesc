# This script is a modified version of FindCMAKE.cmake obtained from 
# svn co https://gforge.sci.utah.edu/svn/findcuda/trunk FindCUDA

# It has been modified to simply find the location of the CUDA runtime
# libraries.
# Modified by Alamelu Sankaranarayanan


################################################################################
# This script locates the NVIDIA CUDA C tools. It should work on linux, windows,
# and mac and should be reasonably up to date with CUDA C releases.
#
# This script makes use of the standard find_package arguments of <VERSION>,
# REQUIRED and QUIET.  CUDA_FOUND will report if an acceptable version of CUDA
# was found.
#
# The script will prompt the user to specify CUDA_TOOLKIT_ROOT_DIR if the prefix
# cannot be determined by the location of nvcc in the system path and REQUIRED
# is specified to find_package(). To use a different installed version of the
# toolkit set the environment variable CUDA_BIN_PATH before running cmake
# (e.g. CUDA_BIN_PATH=/usr/local/cuda1.0 instead of the default /usr/local/cuda)
# or set CUDA_TOOLKIT_ROOT_DIR after configuring.  If you change the value of
# CUDA_TOOLKIT_ROOT_DIR, various components that depend on the path will be
# relocated.
#
# It might be necessary to set CUDA_TOOLKIT_ROOT_DIR manually on certain
# platforms, or to use a cuda runtime not installed in the default location. In
# newer versions of the toolkit the cuda library is included with the graphics
# driver- be sure that the driver version matches what is needed by the cuda
# runtime version.

# The script defines the following variables:
#
#  CUDA_VERSION_MAJOR    -- The major version of cuda as reported by nvcc.
#  CUDA_VERSION_MINOR    -- The minor version.
#  CUDA_VERSION
#  CUDA_VERSION_STRING   -- CUDA_VERSION_MAJOR.CUDA_VERSION_MINOR
#
#  CUDA_TOOLKIT_ROOT_DIR -- Path to the CUDA Toolkit (defined if not set).
#  CUDA_INCLUDE_DIRS     -- Include directory for cuda headers.  Added automatically
#                           for CUDA_ADD_EXECUTABLE and CUDA_ADD_LIBRARY.
#  CUDA_LIBRARIES        -- Cuda RT library.
#  CUDA_CUFFT_LIBRARIES  -- Device or emulation library for the Cuda FFT
#                           implementation (alternative to:
#                           CUDA_ADD_CUFFT_TO_TARGET macro)
#  CUDA_CUBLAS_LIBRARIES -- Device or emulation library for the Cuda BLAS
#                           implementation (alterative to:
#                           CUDA_ADD_CUBLAS_TO_TARGET macro).


# Search for the cuda distribution.
if(NOT CUDA_TOOLKIT_ROOT_DIR)

  # Search in the CUDA_BIN_PATH first.
  find_path(CUDA_TOOLKIT_ROOT_DIR
    NAMES nvcc nvcc.exe
    PATHS ENV CUDA_BIN_PATH
    DOC "Toolkit location."
    NO_DEFAULT_PATH
    )
  # Now search default paths
  find_path(CUDA_TOOLKIT_ROOT_DIR
    NAMES nvcc nvcc.exe
    PATHS /usr/local/bin
          /usr/local/cuda/bin
    DOC "Toolkit location."
    )

  if (CUDA_TOOLKIT_ROOT_DIR)
    string(REGEX REPLACE "[/\\\\]?bin[64]*[/\\\\]?$" "" CUDA_TOOLKIT_ROOT_DIR ${CUDA_TOOLKIT_ROOT_DIR})
    # We need to force this back into the cache.
    set(CUDA_TOOLKIT_ROOT_DIR ${CUDA_TOOLKIT_ROOT_DIR} CACHE PATH "Toolkit location." FORCE)
  endif(CUDA_TOOLKIT_ROOT_DIR)

  if (NOT EXISTS ${CUDA_TOOLKIT_ROOT_DIR})
    if(CUDA_FIND_REQUIRED)
      message(FATAL_ERROR "Specify CUDA_TOOLKIT_ROOT_DIR")
    elseif(NOT CUDA_FIND_QUIETLY)
      message(FATAL_ERROR "CUDA_TOOLKIT_ROOT_DIR not found or specified")
    endif()
  endif (NOT EXISTS ${CUDA_TOOLKIT_ROOT_DIR})

endif (NOT CUDA_TOOLKIT_ROOT_DIR)

# CUDA_NVCC_EXECUTABLE
find_program(CUDA_NVCC_EXECUTABLE
  NAMES nvcc
  PATHS "${CUDA_TOOLKIT_ROOT_DIR}/bin"
        "${CUDA_TOOLKIT_ROOT_DIR}/bin64"
  ENV CUDA_BIN_PATH
  NO_DEFAULT_PATH
  )
# Search default search paths, after we search our own set of paths.
find_program(CUDA_NVCC_EXECUTABLE nvcc)
mark_as_advanced(CUDA_NVCC_EXECUTABLE)

if(CUDA_NVCC_EXECUTABLE AND NOT CUDA_VERSION)
  # Compute the version.
  execute_process (COMMAND ${CUDA_NVCC_EXECUTABLE} "--version" OUTPUT_VARIABLE NVCC_OUT)
  string(REGEX REPLACE ".*release ([0-9]+)\\.([0-9]+).*" "\\1" CUDA_VERSION_MAJOR ${NVCC_OUT})
  string(REGEX REPLACE ".*release ([0-9]+)\\.([0-9]+).*" "\\2" CUDA_VERSION_MINOR ${NVCC_OUT})
  set(CUDA_VERSION "${CUDA_VERSION_MAJOR}.${CUDA_VERSION_MINOR}" CACHE STRING "Version of CUDA as computed from nvcc.")
  mark_as_advanced(CUDA_VERSION)
endif(CUDA_NVCC_EXECUTABLE AND NOT CUDA_VERSION)

# Always set this convenience variable
set(CUDA_VERSION_STRING "${CUDA_VERSION}")

# CUDA_TOOLKIT_INCLUDE
find_path(CUDA_TOOLKIT_INCLUDE
  device_functions.h # Header included in toolkit
  PATHS "${CUDA_TOOLKIT_ROOT_DIR}/include"
  ENV CUDA_INC_PATH
  NO_DEFAULT_PATH
  )
# Search default search paths, after we search our own set of paths.
find_path(CUDA_TOOLKIT_INCLUDE device_functions.h)
mark_as_advanced(CUDA_TOOLKIT_INCLUDE)

# Set the user list of include dir to nothing to initialize it.
set (CUDA_NVCC_INCLUDE_ARGS_USER "")
set (CUDA_INCLUDE_DIRS ${CUDA_TOOLKIT_INCLUDE})

macro(FIND_LIBRARY_LOCAL_FIRST _var _names _doc)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(_cuda_64bit_lib_dir "${CUDA_TOOLKIT_ROOT_DIR}/lib64") 
  endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    #endif()

  find_library(${_var}
    NAMES ${_names}
    PATHS ${_cuda_64bit_lib_dir}
          "${CUDA_TOOLKIT_ROOT_DIR}/lib"
    ENV CUDA_LIB_PATH
    DOC ${_doc}
    NO_DEFAULT_PATH
    )
  # Search default search paths, after we search our own set of paths.
  find_library(${_var} NAMES ${_names} DOC ${_doc})
endmacro(FIND_LIBRARY_LOCAL_FIRST _var _names _doc)
  #endmacro()

# CUDA_LIBRARIES
find_library_local_first(CUDA_CUDART_LIBRARY cudart "\"cudart\" library")

set(CUDA_LIBRARIES ${CUDA_CUDART_LIBRARY})
    
# FIXME : This portion is changed to always point at the 32 bit library since
# that is what we use in eSESC
set(CUDA_LIBRARIES "${CUDA_TOOLKIT_ROOT_DIR}/lib/libcudart.so")

if(APPLE)
  # We need to add the path to cudart to the linker using rpath, since the
  # library name for the cuda libraries is prepended with @rpath.
  get_filename_component(_cuda_path_to_cudart "${CUDA_CUDART_LIBRARY}" PATH)
  if(_cuda_path_to_cudart)
    list(APPEND CUDA_LIBRARIES -Wl,-rpath "-Wl,${_cuda_path_to_cudart}")
  endif(_cuda_path_to_cudart)
endif(APPLE)

mark_as_advanced(
  CUDA_CUDA_LIBRARY
  CUDA_CUDART_LIBRARY
  )

#######################
# Look for some of the toolkit helper libraries
macro(FIND_CUDA_HELPER_LIBS _name)
  find_library_local_first(CUDA_${_name}_LIBRARY ${_name} "\"${_name}\" library")
  mark_as_advanced(CUDA_${_name}_LIBRARY)
endmacro(FIND_CUDA_HELPER_LIBS _name)

# Search for cufft and cublas libraries.
find_cuda_helper_libs(cufftemu)
find_cuda_helper_libs(cublasemu)
find_cuda_helper_libs(cufft)
find_cuda_helper_libs(cublas)

if (CUDA_BUILD_EMULATION)
  set(CUDA_CUFFT_LIBRARIES ${CUDA_cufftemu_LIBRARY})
  set(CUDA_CUBLAS_LIBRARIES ${CUDA_cublasemu_LIBRARY})
  #else()
else (CUDA_BUILD_EMULATION)
  set(CUDA_CUFFT_LIBRARIES ${CUDA_cufft_LIBRARY})
  set(CUDA_CUBLAS_LIBRARIES ${CUDA_cublas_LIBRARY})
endif (CUDA_BUILD_EMULATION)
  #endif()
MESSAGE("-- Found CUDA version ${CUDA_VERSION} : ${CUDA_LIBRARIES}")
#MESSAGE("CUDA LIBS \"${CUDA_LIBRARIES} --version\" ${CUDA_VERSION}\n")
