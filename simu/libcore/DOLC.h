
#ifndef DOLC_H
#define DOLC_H

class DOLC {
private:
  uint64_t depth;
  uint64_t olderBits;
  uint64_t lastBits;
  uint64_t currBits;

  uint64_t *hist;
  uint64_t *histMask;
  uint64_t *histBits;

  void allocate() {
    uint64_t lastMask  = (((uint64_t)1)<<lastBits)-1;
    uint64_t currMask  = (((uint64_t)1)<<currBits)-1;

    hist      = new uint64_t[depth];
    histBits  = new uint64_t[depth];
    histMask  = new uint64_t[depth];

    uint64_t obits = olderBits;
    for(uint64_t i=0;i<depth;i++) {
      hist[i] = 0;
      if ((i==128)
          || (i==256)
          || (i==64)
          || (i==32)
          || (i==3316) ) {
        obits--;
      }
      if (obits<2)
        obits = 2;

      histBits[i] = obits;
      histMask[i] = ((uint64_t)1<<obits)-1;
    }

    histBits[1] = lastBits;
    histMask[1] = lastMask;

    histBits[0] = currBits;
    histMask[0] = currMask;
  }
public:
  DOLC(int d, int o, int l, int c)
    : depth(d)
      , olderBits(o)
      , lastBits(l)
      , currBits(c) {

    if (o>63 || l>63 || c>63) {
      printf("ERROR: DOLC out of limits 64bits per entry\n");
      exit(-1);
    }

    allocate();

    for(uint64_t i=0;i<depth;i++) {
      hist[i] = 0; // Just for being deterministic :)
    }
  }

  DOLC(const DOLC &other) {
    if (depth != other.depth || olderBits != other.olderBits || lastBits != other.lastBits || currBits != other.currBits ) {
      delete hist;
      delete histBits;
      delete histMask;

      depth = other.depth;
      olderBits = other.olderBits;
      lastBits  = other.lastBits;
      currBits  = other.currBits;

      allocate();
    }

    for(uint64_t i=0;i<depth;i++) {
      hist[i] = other.hist[i];
    }
  }

  void mix(uint64_t addr) {

    uint32_t drop  = hist[0]>>10;
    uint32_t sign1 = hist[0] ^ drop ^ (drop<<(5));
    uint32_t sign2 = (sign1<<(5)) + addr;

    hist[0] = sign2;
  }

  void update(uint64_t addr) {

    int duplicate = depth-1; // Start full depth, if no duplicate
    for(int i=duplicate;i>0;i--) {
      hist[i] = hist[i-1];
    }

    hist[0] = addr;
  }

  uint64_t getSign(uint16_t bits, uint16_t m) const {

    uint16_t nbits     = 0;
    uint64_t sign = 0;

    int i_max = m;
    if (i_max>static_cast<int>(depth))
      i_max = depth;

    for(int i=0;i<i_max;i++) {
      uint64_t oBits = histBits[i];
      uint64_t h = hist[i] & histMask[i];

      // Rotate
      uint64_t drop = sign>>(64-oBits);
      sign          = sign ^ drop;
      sign          = (sign<<oBits) + h;

      nbits += oBits;
    }

    if (bits>nbits) {
      int nfolds = bits/nbits;
      for(int i=0;i<nfolds;i++) {
        sign = sign + (sign<<(i*bits));
      }
    }else{
      int nfolds = nbits/bits;

      for(int i=0;i<nfolds;i++) {
        sign = sign + (sign>>(i*bits));
      }
    }

#if 0
    uint64_t key = sign;
    key         += (key << 12);
    key         ^= (key >> 22);
    key         += (key << 4);
    key         ^= (key >> 9);
    key         += (key << 10);
    key         ^= (key >> 2);
    key         += (key << 7);
    key         ^= (key >> 12);
    sign         = key;
#endif

    return sign & ((1<<(bits))-1); 
  }

  void reset(uint64_t sign) {
    for(uint64_t i=0;i<depth;i++) {
      hist[i] = sign & histMask[i];
    }

  }
};

#endif
