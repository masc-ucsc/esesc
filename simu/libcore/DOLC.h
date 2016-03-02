
#ifndef DOLC_H
#define DOLC_H

class DOLC {
private:
  const uint64_t depth;
  const uint64_t olderBits;
  const uint64_t lastBits;
  const uint64_t currBits;

  uint64_t *hist;
  uint64_t *histMask;
  uint64_t *histBits;

  bool     *bias;

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

    uint64_t lastMask  = (((uint64_t)1)<<l)-1;
    uint64_t currMask  = (((uint64_t)1)<<c)-1;

    hist      = new uint64_t[depth];
    histBits  = new uint64_t[depth];
    histMask  = new uint64_t[depth];
    bias      = new bool[depth];

    uint64_t obits = olderBits;
    for(uint64_t i=0;i<depth;i++) {
      bias[i] = false;
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

  uint64_t randomize(uint64_t key, int counter) const {

    key ^= counter;
    key += (key << 12);
    key ^= (key >> 22);
    key += (key << 4);
    key ^= (key >> 9);
    key += (key << 10);
    key ^= (key >> 2);
    key += (key << 7);
    key ^= (key >> 12);

    return key;
  }

  void update(uint64_t addr, bool b) {

#if 0
    static uint64_t last_addr = 0;
    static uint64_t prev_addr = 0;
    static uint64_t last_addr_cntr = 0;
    static uint64_t prev_addr_cntr = 0;

    if (last_addr == addr) {
      last_addr_cntr++;
      printf("+");
      //hist[0] = prev_addr ^ randomize(last_addr,last_addr_cntr);
      return;
    }else if (prev_addr == addr) {
      prev_addr_cntr++;
      printf("-");
      //hist[1] = last_addr ^ randomize(prev_addr,prev_addr_cntr);
      //return;
    }else{
      printf("_");
      last_addr_cntr = 0;
      prev_addr_cntr = 0;
    }

    prev_addr = last_addr;
    last_addr = addr;
#endif

    int duplicate = depth-1; // Start full depth, if no duplicate
#if 0
    // Worse average, but new kind of history (but always worse!)
    for(int i=depth-1;i>0;i--) {
      if (hist[i] == addr && bias[i] == b) {
        duplicate = i;
        break;
      }
    }
#endif
#if 1
    // No difference for analyzed benchmarks (super long may benefit more)
    if (b && bias[0]) {
      // Combine continuous bias jumps to a single signature
      hist[0] = hist[0] ^ (addr>>1);
      return;
    }
#endif

    for(int i=duplicate;i>0;i--) {
      hist[i] = hist[i-1];
      bias[i] = bias[i-1];
    }

    hist[0] = addr;
    bias[0] = b;

#if 0
    int conta[32] = {0,};
    for(int i=0;i<depth;i++) {
      conta[hist[i]%31]++;
    }
    printf("sign %llx: ",addr);
    for(int i=0;i<31;i++) {
      printf("%3d ",conta[i]);
    }
    printf("\n");
#endif
  }
  uint64_t getSignInt(uint64_t hPC, uint16_t bits, uint16_t m) const {
#if 0
    // Half Signature helps alias, and reduces footprint to checkpoint branch predictor
#if 1
    int sa= m;
    if (sa>(bits+3))
      sa = bits-3;
    else 
      sa = sa/2;

    if (sa<3)
      sa = 3;
#else
    int sa = bits/2+2;
#endif

    //printf("bits=%d m=%d sa=%d\n",bits,m,sa);
    uint64_t sign = getSign(sa,m);

    uint64_t s = (hPC<<sa) + (sign & ((1<<sa)-1) );
    sign       = s;
#else
    uint64_t sign = getSign(bits,m);
#endif

    return sign & ((1<<bits)-1);
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
#if 0
      // A bit worse
      oBits++;
      h = (h<<1) + bias[i]?1:0;
#endif

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
      bias[i] = true;
    }

  }
};

#endif
