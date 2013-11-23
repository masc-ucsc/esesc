// Contributed by Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and 
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef ESTL_H
#define ESTL_H

// Diferent compilers have slightly different calling conventions for
// STL. This file has a cross-compiler implementation.

#ifdef __INTEL_COMPILER
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/slist>
#include <algorithm>
#define HASH_MAP       stlport::hash_map
#define HASH_SET       stlport::hash_set
#define HASH_MULTIMAP  stlport::hash_multimap
#define HASH           std::hash
#define SLIST          std::slist
#elif USE_STL_PORT
/* Sun Studio or Standard compiler -library=stlport5 */
#include <hash_map>
#include <hash_set>
#include <slist>
#include <algorithm>
#define HASH_MAP       std::hash_map
#define HASH_SET       std::hash_set
#define HASH_MULTIMAP  std::hash_multimap
#define HASH           std::hash
#define SLIST          std::slist

/* GNU C Compiler */
#elif __GNUC__ == 4

/* gcc 4.2 and earlier */
#if __GNUC_MINOR__ < 3
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/slist>
#include <ext/algorithm>
#define HASH_MAP       __gnu_cxx::hash_map
#define HASH_SET       __gnu_cxx::hash_set
#define HASH_MULTIMAP  __gnu_cxx::hash_multimap
#define HASH           __gnu_cxx::hash
#define SLIST          __gnu_cxx::slist

/* gcc 4.3 */
#elif __GNUC_MINOR__ == 3 
//#include <backward/hash_set>
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/slist>
#include <ext/algorithm>
#define HASH_MAP       __gnu_cxx::hash_map
#define HASH_SET       __gnu_cxx::hash_set
#define HASH_MULTIMAP  __gnu_cxx::hash_multimap
#define HASH           __gnu_cxx::hash
#define SLIST          __gnu_cxx::slist

/* gcc 4.4 and later */
#elif __GNUC_MINOR__ > 3
#include <boost/functional/hash.hpp>
#include <boost/utility.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <ext/slist>
//#include <ext/hash_map>
#define HASH_MAP       boost::unordered_map
#define HASH_SET       boost::unordered_set
#define HASH_MULTIMAP  boost::unordered_multimap
#define HASH           boost::hash
#define SLIST          __gnu_cxx::slist
namespace boost {
  template <> struct hash<const char *> {
    std::size_t
    operator()(const char * __val) const {
      std::string v = static_cast<std::string>(__val);
      return hash_range(v.begin(),v.end());
    }
    };

  template <> struct hash<char *> {
    std::size_t
    operator()(char * __val) const {
      std::string v = static_cast<std::string>(__val);
      return hash_range(v.begin(),v.end());
    }
    };
}

#endif // GNUC_MINOR
#endif // GNUC

#include <boost/dynamic_bitset.hpp>

#endif // ESTL_H
