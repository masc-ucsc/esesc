#if !(defined CVTENDIAN_H)
#define CVTENDIAN_H

#include <errno.h>
#include <stdint.h>

#ifdef DARWIN
#include <machine/endian.h>
#define __BIG_ENDIAN __DARWIN_BIG_ENDIAN
#define __LITTLE_ENDIAN __DARWIN_LITTLE_ENDIAN
#define __BYTE_ORDER __DARWIN_BYTE_ORDER
#else
#include <endian.h>
#endif

typedef float  float32_t;
typedef double float64_t;

#if !(defined __FLOAT_WORD_ORDER)
#define __FLOAT_WORD_ORDER __BYTE_ORDER
#endif

template <typename T, int32_t __SRC_BYTE_ORDER, int32_t __DST_BYTE_ORDER> inline void cvtEndian(T &val) {
  switch(__SRC_BYTE_ORDER) {
  case __BIG_ENDIAN:
  case __LITTLE_ENDIAN:
    break;
  default:
    errno = EINVAL;
    perror("__BYTE_ORDER (host machine's byte order) is unknown\n");
    exit(1);
  }
  switch(__DST_BYTE_ORDER) {
  case __BIG_ENDIAN:
  case __LITTLE_ENDIAN:
    break;
  default:
    errno = EINVAL;
    perror("__TARG_BYTE_ORDER (simulated machine's byte order) is unknown\n");
    exit(1);
  }
  if(__SRC_BYTE_ORDER == __DST_BYTE_ORDER) {
    // Do nothing
  } else {
    switch(sizeof(val)) {
    case 8:
      val = (((val / 0x0000000100000000ull) & 0x00000000ffffffffull) + ((val & 0x00000000ffffffffull) * 0x0000000100000000ull));
      val = (((val >> 16) & 0x0000ffff0000ffffull) | ((val & 0x0000ffff0000ffffull) << 16));
      val = (((val >> 8) & 0x00ff00ff00ff00ffull) | ((val & 0x00ff00ff00ff00ffull) << 8));
      break;
    case 4:
      val = (((val >> 16) & 0x0000fffful) | ((val & 0x0000fffful) << 16));
      val = (((val >> 8) & 0x00ff00fful) | ((val & 0x00ff00fful) << 8));
      break;
    case 2:
      val = (((val >> 8) & 0x00ffu) | ((val & 0x00fful) << 8));
      break;
    case 1:
      break;
    default:
      errno = EINVAL;
      perror("bigEndian called with wrong-size argument");
      exit(1);
    }
  }
}

template <typename T> inline void cvtEndian(T &val, int32_t byteOrder) {
  if(byteOrder == __BIG_ENDIAN)
    cvtEndian<T, __BIG_ENDIAN, __BYTE_ORDER>(val);
  else if(byteOrder == __LITTLE_ENDIAN)
    cvtEndian<T, __LITTLE_ENDIAN, __BYTE_ORDER>(val);
  else {
    errno = EINVAL;
    perror("cvtEndian with invalid byteOrder");
    exit(1);
  }
}

template <typename T> inline void cvtEndianBig(T &val) {
  cvtEndian<T, __BIG_ENDIAN, __BYTE_ORDER>(val);
}
template <> inline void cvtEndianBig(float64_t &val) {
  cvtEndian<uint64_t, __BIG_ENDIAN, __FLOAT_WORD_ORDER>(*((uint64_t *)(&val)));
}
template <> inline void cvtEndianBig(float32_t &val) {
  cvtEndian<uint32_t, __BIG_ENDIAN, __FLOAT_WORD_ORDER>(*((uint32_t *)(&val)));
}

#endif
