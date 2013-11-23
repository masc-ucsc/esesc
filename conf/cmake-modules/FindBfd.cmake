# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Finds BFD library
#
#  Bfd_INCLUDE_DIR - where to find bfd.h, etc.
#  Bfd_LIBRARIES   - List of libraries when using bfd.
#  Bfd_FOUND       - True if bfd found.


if (Bfd_INCLUDE_DIR)
  # Already in cache, be silent
  set(Bfd_FIND_QUIETLY TRUE)
endif (Bfd_INCLUDE_DIR)

find_path(Bfd_INCLUDE_DIR bfd.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Bfd_NAMES bfd)
find_library(Bfd_LIBRARY
  NAMES ${Bfd_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Bfd_INCLUDE_DIR AND Bfd_LIBRARY)
   set(Bfd_FOUND TRUE)
   set( Bfd_LIBRARIES ${Bfd_LIBRARY} )
else (Bfd_INCLUDE_DIR AND Bfd_LIBRARY)
   set(Bfd_FOUND FALSE)
   set(Bfd_LIBRARIES)
endif (Bfd_INCLUDE_DIR AND Bfd_LIBRARY)

if (Bfd_FOUND)
   if (NOT Bfd_FIND_QUIETLY)
      message(STATUS "Found BFD: ${Bfd_LIBRARY}")
   endif (NOT Bfd_FIND_QUIETLY)
else (Bfd_FOUND)
   if (Bfd_FIND_REQUIRED)
      message(STATUS "Looked for Bfd libraries named ${Bfd_NAMES}.")
      message(FATAL_ERROR "Could NOT find Bfd library")
   endif (Bfd_FIND_REQUIRED)
endif (Bfd_FOUND)

mark_as_advanced(
  Bfd_LIBRARY
  Bfd_INCLUDE_DIR
  )
