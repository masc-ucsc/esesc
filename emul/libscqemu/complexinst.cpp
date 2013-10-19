#include <stdio.h>
#include "complexinst.h"
#include "InstOpcode.h"
#include "qemumin.h"

#define SIGNBIT   (uint32_t)0x80000000
#define SIGNBIT64 ((uint64_t)1 << 63)

#define INT64_MAX  0x7FFFFFFFFFFFFFFFLL
#define UNSIGNEDMAX64 0xFFFFFFFFFFFFFFFFULL

uint32_t do_clz(ProgramBase *prog, uint32_t arg1) {
  uint32_t val = prog->ldu32( arg1+4 ) ;
  // do clz
  int cnt = 0;

  if (!(val & 0xFFFF0000U)) {
    cnt += 16;
    val <<= 16;
  }
  if (!(val & 0xFF000000U)) {
    cnt += 8;
    val <<= 8;
  }
  if (!(val & 0xF0000000U)) {
    cnt += 4;
    val <<= 4;
  }
  if (!(val & 0xC0000000U)) {
    cnt += 2;
    val <<= 2;
  }
  if (!(val & 0x80000000U)) {
    cnt++;
    val <<= 1;
  }
  if (!(val & 0x80000000U)) {
    cnt++;
  }

  prog->st32( arg1 + 16, cnt ) ;

  return 0;
}

uint32_t do_mrc(ProgramBase *prog, uint32_t arg1) {

  uint32_t CRN = prog->ldu32( arg1+4 ) ;

  if(CRN == 0) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 1) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 2) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 3) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 4) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 5) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 6) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 7) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 8) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 9) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 10) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 11) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 12) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 13) {
     uint32_t val = prog->ldu32( arg1 + CVAR_TLS2_OFFST*sizeof(uint64_t)) ;
    prog->st32( arg1 + 16, val ) ;
  }
  else if(CRN == 14) {
    printf("NOT IMPLEMENTED.\n");
  }
  else if(CRN == 15) {
    printf("NOT IMPLEMENTED.\n");
  }

  return 0;
}

uint32_t do_vabs(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low      = prog->ldu32( arg1+4 ) ;
  uint32_t high     = prog->ldu32( arg1+8 ) ;

  uint32_t imm      = prog->ldu32( arg1+12 ) ;

  uint32_t quadlow;
  uint32_t quadhigh;
  uint32_t temp = 0;

  uint32_t lresult, hresult, qlresult, qhresult;

  uint8_t size = imm & 0x3;
  uint8_t quad = (imm>>2) & 0x1;
  uint8_t saturate = (imm>>3) & 0x1;

  uint8_t  eight;
  uint16_t sixteen;

  uint8_t x8;
  uint16_t x16;
  uint32_t x32;


  uint8_t i;
  uint8_t sat = 0;

  if(quad) {
    quadlow  = prog->ldu32( arg1+36 ) ;
    quadhigh = prog->ldu32( arg1+40 ) ;
  }

  if(!saturate) { //VABS
    switch(size) {
      case 0: //8-bits
        for(i = 0; i < 4; i++) {
          eight = (low >> (8*i)) & 0xFF; 

          if((int8_t)eight < 0)
            eight = -((int8_t)eight);

          temp |= eight << (8*i);
        }

        low = temp;
        temp = 0;

        for(i = 0; i < 4; i++) {
          eight = (high >> (8*i)) & 0xFF; 

          if((int8_t)eight < 0)
            eight = -((int8_t)eight);

          temp |= eight << (8*i);
        }

        high = temp;

        if(quad) {
          temp = 0;
          for(i = 0; i < 4; i++) {
            eight = (quadlow >> (8*i)) & 0xFF; 

            if((int8_t)eight < 0)
              eight = -((int8_t)eight);

            temp |= eight << (8*i);
          }

          quadlow = temp;
          temp = 0;

          for(i = 0; i < 4; i++) {
            eight = (quadhigh >> (8*i)) & 0xFF; 

            if((int8_t)eight < 0)
              eight = -((int8_t)eight);

            temp |= eight << (8*i);
          }

          quadhigh = temp;
        }
        break;
      case 1: //16-bits
        for(i = 0; i < 2; i++) {
          sixteen = (low >> (16*i)) & 0xFFFF; 

          if((int16_t)sixteen < 0)
            sixteen = -((int16_t)sixteen);

          temp |= sixteen << (16*i);
        }

        low = temp;
        temp = 0;

        for(i = 0; i < 2; i++) {
          sixteen = (high >> (16*i)) & 0xFFFF; 

          if((int16_t)sixteen < 0)
            sixteen = -((int16_t)sixteen);

          temp |= sixteen << (16*i);
        }

        high = temp;

        if(quad) {
          temp = 0;
          for(i = 0; i < 2; i++) {
            sixteen = (quadlow >> (16*i)) & 0xFFFF; 

            if((int16_t)sixteen < 0)
              sixteen = -((int16_t)sixteen);

            temp |= sixteen << (16*i);
          }

          quadlow = temp;
          temp = 0;

          for(i = 0; i < 2; i++) {
            sixteen = (quadhigh >> (16*i)) & 0xFFFF; 

            if((int16_t)sixteen < 0)
              sixteen = -((int16_t)sixteen);

            temp |= sixteen << (16*i);
          }

          quadhigh = temp;
        }
        break;
      case 2: //32-bits
        if((int32_t)low < 0)
          low = -((int32_t)low);

        if((int32_t)high < 0)
          high = -((int32_t)high);

        if(quad) {
          if((int32_t)quadlow < 0)
            quadlow = -((int32_t)quadlow);

          if((int32_t)high < 0)
            quadhigh = -((int32_t)quadhigh);
        }
        break;
    }

    prog->st32( arg1 + 16, low) ;
    prog->st32( arg1 + 20, high ) ;

    if(quad) {
      prog->st32( arg1 + 24, quadlow) ;
      prog->st32( arg1 + 28, quadhigh ) ;
    }
  }
  else { //VQABS
    switch(size) {
      case 0: //8-bits

        for(i = 0; i < 4; i++) {
          x8 = (low>>(i*8)) & 0xFF;

          if (x8 == (int8_t)0x80) {
            x8 = 0x7f;
            sat = 1;
          } 
          else if (x8 < 0) {
            x8 = -x8;
          }

          lresult |= (x8 & 0xFF) <<(i*8);
        }

        for(i = 0; i < 4; i++) {
          x8 = (high>>(i*8)) & 0xFF;

          if (x8 == (int8_t)0x80) {
            x8 = 0x7f;
            sat = 1;
          } 
          else if (x8 < 0) {
            x8 = -x8;
          }

          hresult |= (x8 & 0xFF) <<(i*8);
        }

        prog->st32( arg1 + 16, lresult );
        prog->st32( arg1 + 20, hresult );

        if(!quad) 
          prog->st32( arg1 + 24, sat);

        if(quad) {
          for(i = 0; i < 4; i++) {
            x8 = (quadlow>>(i*8)) & 0xFF;

            if (x8 == (int8_t)0x80) {
              x8 = 0x7f;
              sat = 1;
            } 
            else if (x8 < 0) {
              x8 = -x8;
            }

            qlresult |= (x8 & 0xFF) <<(i*8);
          }

          for(i = 0; i < 4; i++) {
            x8 = (quadhigh>>(i*8)) & 0xFF;

            if (x8 == (int8_t)0x80) {
              x8 = 0x7f;
              sat = 1;
            } 
            else if (x8 < 0) {
              x8 = -x8;
            }

            qhresult |= (x8 & 0xFF) <<(i*8);
          }

          prog->st32( arg1 + 24, qlresult );
          prog->st32( arg1 + 28, qhresult );
          prog->st32( arg1 + 32, sat);
        }
        break;
      case 1: //16-bits
        for(i = 0; i < 2; i++) {
          x16 = (low >> (i*16)) & 0xFFFF;

          if (x16 == (int16_t)0x8000) {
            x16 = 0x7fff;
            sat = 1;
          } 
          else if (x16 < 0) {
            x16 = -x16;
          }

          lresult |= (x16 & 0xFFFF) << (i*16);
        }

        for(i = 0; i < 2; i++) {
          x16 = (high >> (i*16)) & 0xFFFF;

          if (x16 == (int16_t)0x8000) {
            x16 = 0x7fff;
            sat = 1;
          } 
          else if (x16 < 0) {
            x16 = -x16;
          }

          hresult |= (x16 & 0xFFFF) << (i*16);
        }

        prog->st32( arg1 + 16, lresult );
        prog->st32( arg1 + 20, hresult );

        if(!quad) 
          prog->st32( arg1 + 24, sat);

        if(quad) {
          for(i = 0; i < 2; i++) {
            x16 = (quadlow >> (i*16)) & 0xFFFF;

            if (x16 == (int16_t)0x8000) {
              x16 = 0x7fff;
              sat = 1;
            } 
            else if (x16 < 0) {
              x16 = -x16;
            }

            qlresult |= (x16 & 0xFFFF) << (i*16);
          }

          for(i = 0; i < 2; i++) {
            x16 = (quadhigh >> (i*16)) & 0xFFFF;

            if (x16 == (int16_t)0x8000) {
              x16 = 0x7fff;
              sat = 1;
            } 
            else if (x16 < 0) {
              x16 = -x16;
            }

            qhresult |= (x16 & 0xFFFF) << (i*16);
          }

          prog->st32( arg1 + 24, qlresult );
          prog->st32( arg1 + 28, qhresult );
          prog->st32( arg1 + 32, sat);
        }
        break;
      case 2: //32-bits
        x32 = low;
        if (x32 == SIGNBIT) {
          sat = 1;
          x32 = ~SIGNBIT;
        } 
        else if ((int32_t)x32 < 0) {
          x32 = -x32;
        }

        prog->st32( arg1 + 16, x32 );

        x32 = high;
        if (x32 == SIGNBIT) {
          sat = 1;
          x32 = ~SIGNBIT;
        } 
        else if ((int32_t)x32 < 0) {
          x32 = -x32;
        }

        prog->st32( arg1 + 20, x32 );

        if(!quad) 
          prog->st32( arg1 + 24, sat);

        if(quad) { 
          x32 = quadlow;
          if (x32 == SIGNBIT) {
            sat = 1;
            x32 = ~SIGNBIT;
          } 
          else if ((int32_t)x32 < 0) {
            x32 = -x32;
          }

          prog->st32( arg1 + 24, x32 );

          x32 = quadhigh;
          if (x32 == SIGNBIT) {
            sat = 1;
            x32 = ~SIGNBIT;
          } 
          else if ((int32_t)x32 < 0) {
            x32 = -x32;
          }

          prog->st32( arg1 + 28, x32 );
          prog->st32( arg1 + 32, sat);
        }
        break;
    }
  }
  return 0;
}

uint32_t do_vaba(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low1     = prog->ldu32( arg1+4 ) ;
  uint32_t high1    = prog->ldu32( arg1+8 ) ;
  uint32_t imm      = prog->ldu32( arg1+12 ) ;
  uint32_t acclow   = prog->ldu32( arg1+16 ) ;
  uint32_t acchigh  = prog->ldu32( arg1+20 ) ;

  uint32_t low2;
  uint32_t high2;

  uint32_t quadlow1;
  uint32_t quadhigh1;
  uint32_t quadlow2;
  uint32_t quadhigh2;
  uint32_t quadacclow;
  uint32_t quadacchigh;

  uint32_t temp = 0;

  uint8_t size = imm & 0x3;
  uint8_t quad = (imm>>2) & 0x1;
  uint8_t is_unsigned = (imm>>3) & 0x1;
  uint8_t is_long = (imm>>4) & 0x1;

  uint8_t  eight1, eight2, temp8, acc8;
  uint16_t sixteen1, sixteen2, temp16, acc16;

  uint32_t low;
  uint32_t high;
  uint32_t quadlow;
  uint32_t quadhigh;

  uint64_t acc64U;
  int64_t acc64S;

  uint64_t temp64U;
  int64_t temp64S;

  if(quad || is_long) {
    low2        = prog->ldu32( arg1+44 ) ;
    high2       = prog->ldu32( arg1+48 ) ;

    quadacclow  = prog->ldu32( arg1+24 ) ;
    quadacchigh = prog->ldu32( arg1+28 ) ;

    quadlow1    = prog->ldu32( arg1+36 ) ;
    quadhigh1   = prog->ldu32( arg1+40 ) ;

    quadlow2    = prog->ldu32( arg1+52 ) ;
    quadhigh2   = prog->ldu32( arg1+56 ) ;
  }
  else {
    low2  = prog->ldu32( arg1+28 ) ;
    high2 = prog->ldu32( arg1+32 ) ;
  }

  // do VABA
  switch(size) {
    case 0: //8-bits
      if(is_unsigned) {
        if(!is_long) {
          for(int i = 0; i < 4; i++) {
            eight1 = (low1 >> (8*i)) & 0xFF;
            eight2 = (low2 >> (8*i)) & 0xFF;
            acc8 = (acclow >> (8*i)) & 0xFF;
            
            temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
            temp8 += acc8;
            temp |= temp8 << (8*i);
          }

          low = temp;
          temp = 0;

          for(int i = 0; i < 4; i++) {
            eight1 = (high1 >> (8*i)) & 0xFF;
            eight2 = (high2 >> (8*i)) & 0xFF;
            acc8 = (acchigh >> (8*i)) & 0xFF;
            
            temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
            temp8 += acc8;
            temp |= (uint32_t)temp8 << (8*i);
          }

          high = temp;

          if(quad) {
            temp = 0;
            for(int i = 0; i < 4; i++) {
              eight1 = (quadlow1 >> (8*i)) & 0xFF;
              eight2 = (quadlow2 >> (8*i)) & 0xFF;
              acc8 = (quadacclow >> (8*i)) & 0xFF;
              
              temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
              temp8 += acc8;
              temp |= (uint32_t)temp8 << (8*i);
            }

            quadlow = temp;
            temp = 0;

            for(int i = 0; i < 2; i++) {
              eight1 = (quadhigh1 >> (8*i)) & 0xFF;
              eight2 = (quadhigh2 >> (8*i)) & 0xFF;
              acc8 = (quadacchigh >> (8*i)) & 0xFF;
              
              temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
              temp8 += acc8;
              temp |= (uint32_t)temp8 << (8*i);
            }

            quadhigh = temp;
          }
        }
        else {
          eight1 = low1 & 0xFF;
          eight2 = low2 & 0xFF;
          acc16 = acclow & 0xFFFF;
          
          temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp16 = acc16 + temp8;
          temp = temp16;

          eight1 = (low1>>8) & 0xFF;
          eight2 = (low2>>8) & 0xFF;
          acc16 = (acclow>>16) & 0xFFFF;
          
          temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp16 = acc16 + temp8;
          low = ((uint32_t)temp16<<16) | temp;

          eight1 = (low1>>16) & 0xFF;
          eight2 = (low2>>16) & 0xFF;
          acc16 = acchigh & 0xFFFF;
          
          temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp16 = acc16 + temp8;
          temp = temp16;

          eight1 = (low1>>24) & 0xFF;
          eight2 = (low2>>24) & 0xFF;
          acc16 = (acchigh>>16) & 0xFFFF;
          
          temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp16 = acc16 + temp8;
          high = ((uint32_t)temp16<<16) | temp;

          temp64U = ((uint64_t)high << 32) | (uint64_t)low;
          prog->st64( arg1 + 16, temp64U);

          eight1 = high1 & 0xFF;
          eight2 = high2 & 0xFF;
          acc16 = quadacclow & 0xFFFF;
          
          temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp16 = acc16 + temp8;
          temp = temp16;

          eight1 = (high1>>8) & 0xFF;
          eight2 = (high2>>8) & 0xFF;
          acc16 = (quadacclow>>16) & 0xFFFF;
          
          temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp16 = acc16 + temp8;
          quadlow = ((uint32_t)temp16<<16) | temp;

          eight1 = (high1>>16) & 0xFF;
          eight2 = (high2>>16) & 0xFF;
          acc16 = quadacchigh & 0xFFFF;
          
          temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp16 = acc16 + temp8;
          temp = temp16;

          eight1 = (high1>>24) & 0xFF;
          eight2 = (high2>>24) & 0xFF;
          acc16 = (quadacchigh>>16) & 0xFFFF;
          
          temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp16 = acc16 + temp8;
          quadhigh = ((uint32_t)temp16<<16) | temp;

          temp64U = ((uint64_t)quadhigh << 32) | (uint64_t)quadlow;
          prog->st64( arg1 + 24, temp64U);
        }
      }
      else {
        if(!is_long) {
          for(int i = 0; i < 4; i++) {
            eight1 = (low1 >> (8*i)) & 0xFF;
            eight2 = (low2 >> (8*i)) & 0xFF;
            acc8 = (acclow >> (8*i)) & 0xFF;
            
            temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
            temp8 += acc8;
            temp |= temp8 << (8*i);
          }

          low = temp;
          temp = 0;

          for(int i = 0; i < 4; i++) {
            eight1 = (high1 >> (8*i)) & 0xFF;
            eight2 = (high2 >> (8*i)) & 0xFF;
            acc8 = (acchigh >> (8*i)) & 0xFF;
            
            temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
            temp8 += acc8;
            temp |= temp8 << (8*i);
          }

          high = temp;

          if(quad) {
            temp = 0;
            for(int i = 0; i < 4; i++) {
              eight1 = (quadlow1 >> (8*i)) & 0xFF;
              eight2 = (quadlow2 >> (8*i)) & 0xFF;
              acc8 = (quadacclow >> (8*i)) & 0xFF;
              
              temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
              temp8 += acc8;
              temp |= temp8 << (8*i);
            }

            quadlow = temp;
            temp = 0;

            for(int i = 0; i < 2; i++) {
              eight1 = (quadhigh1 >> (8*i)) & 0xFF;
              eight2 = (quadhigh2 >> (8*i)) & 0xFF;
              acc8 = (quadacchigh >> (8*i)) & 0xFF;
              
              temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
              temp8 += acc8;
              temp |= temp8 << (8*i);
            }

            quadhigh = temp;
          }
        }
        else {
          eight1 = low1 & 0xFF;
          eight2 = low2 & 0xFF;
          acc16 = acclow & 0xFFFF;
          
          temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp16 = (int16_t)acc16 + (int16_t)temp8;
          temp = temp16;

          eight1 = (low1>>8) & 0xFF;
          eight2 = (low2>>8) & 0xFF;
          acc16 = (acclow>>16) & 0xFFFF;
          
          temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp16 = (int16_t)acc16 + (int16_t)temp8;
          low = ((uint32_t)temp16<<16) | temp;

          eight1 = (low1>>16) & 0xFF;
          eight2 = (low2>>16) & 0xFF;
          acc16 = acchigh & 0xFFFF;
          
          temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp16 = (int16_t)acc16 + (int16_t)temp8;
          temp = temp16;

          eight1 = (low1>>24) & 0xFF;
          eight2 = (low2>>24) & 0xFF;
          acc16 = (acchigh>>16) & 0xFFFF;
          
          temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp16 = (int16_t)acc16 + (int16_t)temp8;
          high = ((uint32_t)temp16<<16) | temp;

          temp64S = ((uint64_t)high << 32) | (uint64_t)low;
          prog->st64( arg1 + 16, temp64S);

          eight1 = high1 & 0xFF;
          eight2 = high2 & 0xFF;
          acc16 = quadacclow & 0xFFFF;
          
          temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp16 = (int16_t)acc16 + (int16_t)temp8;
          temp = temp16;

          eight1 = (high1>>8) & 0xFF;
          eight2 = (high2>>8) & 0xFF;
          acc16 = (quadacclow>>16) & 0xFFFF;
          
          temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp16 = (int16_t)acc16 + (int16_t)temp8;
          quadlow = ((uint32_t)temp16<<16) | temp;

          eight1 = (high1>>16) & 0xFF;
          eight2 = (high2>>16) & 0xFF;
          acc16 = quadacchigh & 0xFFFF;
          
          temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp16 = (int16_t)acc16 + (int16_t)temp8;
          temp = temp16;

          eight1 = (high1>>24) & 0xFF;
          eight2 = (high2>>24) & 0xFF;
          acc16 = (quadacchigh>>16) & 0xFFFF;
          
          temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp16 = (int16_t)acc16 + (int16_t)temp8;
          quadhigh = ((uint32_t)temp16<<16) | temp;

          temp64S = ((uint64_t)quadhigh << 32) | (uint64_t)quadlow;
          prog->st64( arg1 + 24, temp64S);
        }
      }
      break;
    case 1: //16-bits
      if(is_unsigned) {
        if(!is_long) {
          for(int i = 0; i < 2; i++) {
            sixteen1 = (low1 >> (16*i)) & 0xFFFF;
            sixteen2 = (low2 >> (16*i)) & 0xFFFF;
            acc16 = (acclow >> (16*i)) & 0xFFFF;
            
            temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
            temp16 += acc16;
            temp |= (uint32_t)temp16 << (16*i);
          }

          low = temp;
          temp = 0;

          for(int i = 0; i < 2; i++) {
            sixteen1 = (high1 >> (16*i)) & 0xFFFF;
            sixteen2 = (high2 >> (16*i)) & 0xFFFF;
            acc16 = (acchigh >> (16*i)) & 0xFFFF;
            
            temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
            temp16 += acc16;
            temp |= (uint32_t)temp16 << (16*i);
          }

          high = temp;

          if(quad) {
            temp = 0;
            for(int i = 0; i < 2; i++) {
              sixteen1 = (quadlow1 >> (16*i)) & 0xFFFF;
              sixteen2 = (quadlow2 >> (16*i)) & 0xFFFF;
              acc16 = (quadacclow >> (16*i)) & 0xFFFF;
              
              temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
              temp16 += acc16;
              temp |= (uint32_t)temp16 << (16*i);
            }

            quadlow = temp;
            temp = 0;

            for(int i = 0; i < 2; i++) {
              sixteen1 = (quadhigh1 >> (16*i)) & 0xFFFF;
              sixteen2 = (quadhigh2 >> (16*i)) & 0xFFFF;
              acc16 = (quadacchigh >> (16*i)) & 0xFFFF;
              
              temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
              temp16 += acc16;
              temp |= (uint32_t)temp16 << (16*i);
            }

            quadhigh = temp;
          }
        }
        else {
          sixteen1 = low1 & 0xFFFF;
          sixteen2 = low2 & 0xFFFF;
          
          temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
          low = acclow + temp16;

          sixteen1 = (low1>>16) & 0xFFFF;
          sixteen2 = (low2>>16) & 0xFFFF;
          
          temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
          high = acchigh + temp16;

          temp64U = ((uint64_t)high << 32) | (uint64_t)low;
          prog->st64( arg1 + 16, temp64U);

          sixteen1 = high1 & 0xFFFF;
          sixteen2 = high2 & 0xFFFF;
          
          temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
          quadlow = quadacclow + temp16;

          sixteen1 = (high1>>16) & 0xFFFF;
          sixteen2 = (high2>>16) & 0xFFFF;
          
          temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
          quadhigh = quadacchigh + temp16;

          temp64U = ((uint64_t)quadhigh << 32) | (uint64_t)quadlow;
          prog->st64( arg1 + 24, temp64U);
        }
      }
      else {
        if(!is_long) {
          for(int i = 0; i < 2; i++) {
            sixteen1 = (low1 >> (16*i)) & 0xFFFF;
            sixteen2 = (low2 >> (16*i)) & 0xFFFF;
            acc16 = (acclow >> (16*i)) & 0xFFFF;
            
            temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen1 - (int16_t)sixteen1);
            temp16 += acc16;
            temp |= (uint32_t)temp16 << (16*i);
          }

          low = temp;
          temp = 0;

          for(int i = 0; i < 2; i++) {
            sixteen1 = (high1 >> (16*i)) & 0xFFFF;
            sixteen2 = (high2 >> (16*i)) & 0xFFFF;
            acc16 = (acchigh >> (16*i)) & 0xFFFF;
            
            temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
            temp16 += acc16;
            temp |= (uint32_t)temp16 << (16*i);
          }

          high = temp;

          if(quad) {
            temp = 0;
            for(int i = 0; i < 2; i++) {
              sixteen1 = (quadlow1 >> (16*i)) & 0xFFFF;
              sixteen2 = (quadlow2 >> (16*i)) & 0xFFFF;
              acc16 = (quadacclow >> (16*i)) & 0xFFFF;
              
              temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
              temp16 += acc16;
              temp |= (uint32_t)temp16 << (16*i);
            }

            quadlow = temp;
            temp = 0;

            for(int i = 0; i < 2; i++) {
              sixteen1 = (quadhigh1 >> (16*i)) & 0xFFFF;
              sixteen2 = (quadhigh2 >> (16*i)) & 0xFFFF;
              acc16 = (quadacchigh >> (16*i)) & 0xFFFF;
              
              temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
              temp16 += acc16;
              temp |= (uint32_t)temp16 << (16*i);
            }

            quadhigh = temp;
          }
        }
        else {
          sixteen1 = low1 & 0xFFFF;
          sixteen2 = low2 & 0xFFFF;
          
          temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
          low = (int32_t)acclow + (int16_t)temp16;

          sixteen1 = (low1>>16) & 0xFFFF;
          sixteen2 = (low2>>16) & 0xFFFF;
          
          temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
          high = (int32_t)acchigh + (int16_t)temp16;

          temp64S = ((uint64_t)high << 32) | (uint64_t)low;
          prog->st64( arg1 + 16, temp64S);

          sixteen1 = high1 & 0xFFFF;
          sixteen2 = high2 & 0xFFFF;
          
          temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
          quadlow = (int32_t)quadacclow + (int16_t)temp16;

          sixteen1 = (high1>>16) & 0xFFFF;
          sixteen2 = (high2>>16) & 0xFFFF;
          
          temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
          quadhigh = (int32_t)quadacchigh + (int16_t)temp16;

          temp64S = ((uint64_t)quadhigh << 32) | (uint64_t)quadlow;
          prog->st64( arg1 + 24, temp64S);
        }
      }
      break;
    case 2: //32-bits
      if(is_unsigned) {
        if(!is_long) {
          low = (low1 > low2) ? (low1 - low2) : (low2 - low1);
          low += acclow;

          high = (high1 > high2) ? (high1 - high2) : (high2 - high1);
          high += acchigh;

          if(quad) {
            quadlow = (quadlow1 > quadlow2) ? (quadlow1 - quadlow2) : (quadlow2 - quadlow1);
            quadlow += quadacclow;

            quadhigh = (quadhigh1 > quadhigh2) ? (quadhigh1 - quadhigh2) : (quadhigh2 - quadhigh1);
            quadhigh += quadacchigh;
          }
        }
        else {
          low = (low1 > low2) ? (low1 - low2) : (low2 - low1);
          acc64U = (uint64_t)acchigh << 32 | (uint64_t)acclow;
          temp64U = low + acc64U;
          prog->st64( arg1 + 16, temp64U);

          
          high = (high1 > high2) ? (high1 - high2) : (high2 - high1);
          acc64U = (uint64_t)quadacchigh << 32 | (uint64_t)quadacclow;
          temp64U = high + acc64U;
          prog->st64( arg1 + 24, temp64U);
        }
      }
      else {
        if(!is_long) {
          low = ((int32_t)low1 > (int32_t)low2) ? ((int32_t)low1 - (int32_t)low2) : ((int32_t)low2 - (int32_t)low1);
          low += acclow;

          high = ((int32_t)high1 > (int32_t)high2) ? ((int32_t)high1 - (int32_t)high2) : ((int32_t)high2 - (int32_t)high1);
          high += acchigh;

          if(quad) {
            quadlow = ((int32_t)quadlow1 > (int32_t)quadlow2) ? ((int32_t)quadlow1 - (int32_t)quadlow2) : ((int32_t)quadlow2 - (int32_t)quadlow1);
            quadlow += quadacclow;

            quadhigh = ((int32_t)quadhigh1 > (int32_t)quadhigh2) ? ((int32_t)quadhigh1 - (int32_t)quadhigh2) : ((int32_t)quadhigh2 - (int32_t)quadhigh1);
            quadhigh += quadacchigh;
          }
        }
        else {
          low = ((int32_t)low1 > (int32_t)low2) ? ((int32_t)low1 - (int32_t)low2) : ((int32_t)low2 - (int32_t)low1);
          acc64S = (int64_t)acchigh << 32 | (int64_t)acclow;
          temp64S = low + acc64S;
          prog->st64( arg1 + 16, temp64S);

          
          high = ((int32_t)high1 > (int32_t)high2) ? ((int32_t)high1 - (int32_t)high2) : ((int32_t)high2 - (int32_t)high1);
          acc64S = (int64_t)quadacchigh << 32 | (int64_t)quadacclow;
          temp64S = high + acc64S;
          prog->st64( arg1 + 24, temp64S);
        }
      }
      break;
  }

  if(!is_long) {
    prog->st32( arg1 + 16, low);
    prog->st32( arg1 + 20, high );

    if(quad) {
      prog->st32( arg1 + 24, quadlow);
      prog->st32( arg1 + 28, quadhigh );
    }
  }

  return 0;
}

uint32_t do_vabd(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low1     = prog->ldu32( arg1+4 ) ;
  uint32_t high1    = prog->ldu32( arg1+8 ) ;
  uint32_t imm      = prog->ldu32( arg1+12 ) ;

  uint32_t low2;
  uint32_t high2;

  uint32_t quadlow1;
  uint32_t quadhigh1;
  uint32_t quadlow2;
  uint32_t quadhigh2;

  uint32_t temp = 0;

  uint8_t size = imm & 0x3;
  uint8_t quad = (imm>>2) & 0x1;
  uint8_t is_unsigned = (imm>>3) & 0x1;
  uint8_t is_long = (imm>>4) & 0x1;

  uint8_t  eight1, eight2, temp8;
  uint16_t sixteen1, sixteen2, temp16;

  uint32_t low;
  uint32_t high;
  uint32_t quadlow;
  uint32_t quadhigh;

  uint64_t temp64U;
  int64_t temp64S;

  if(quad || is_long) {
    low2      = prog->ldu32( arg1+44 ) ;
    high2     = prog->ldu32( arg1+48 ) ;

    quadlow1  = prog->ldu32( arg1+36 ) ;
    quadhigh1 = prog->ldu32( arg1+40 ) ;

    quadlow2  = prog->ldu32( arg1+52 ) ;
    quadhigh2 = prog->ldu32( arg1+56 ) ;
  }
  else {
    low2  = prog->ldu32( arg1+28 ) ;
    high2 = prog->ldu32( arg1+32 ) ;
  }

  // do VABD
  switch(size) {
    case 0: //8-bits
      if(is_unsigned) {
        if(!is_long) {
          for(int i = 0; i < 4; i++) {
            eight1 = (low1 >> (8*i)) & 0xFF;
            eight2 = (low2 >> (8*i)) & 0xFF;
            
            temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
            temp |= temp8 << (8*i);
          }

          low = temp;
          temp = 0;

          for(int i = 0; i < 4; i++) {
            eight1 = (high1 >> (8*i)) & 0xFF;
            eight2 = (high2 >> (8*i)) & 0xFF;
            
            temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
            temp |= temp8 << (8*i);
          }

          high = temp;

          if(quad) {
            temp = 0;
            for(int i = 0; i < 4; i++) {
              eight1 = (quadlow1 >> (8*i)) & 0xFF;
              eight2 = (quadlow2 >> (8*i)) & 0xFF;
              
              temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
              temp |= temp8 << (8*i);
            }

            quadlow = temp;
            temp = 0;

            for(int i = 0; i < 2; i++) {
              eight1 = (quadhigh1 >> (8*i)) & 0xFF;
              eight2 = (quadhigh2 >> (8*i)) & 0xFF;
              
              temp8 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
              temp |= temp8 << (8*i);
            }

            quadhigh = temp;
          }
        }
        else {
          eight1 = low1 & 0xFF;
          eight2 = low2 & 0xFF;
          
          temp16 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp = temp16 & 0xFFFF;

          eight1 = (low1>>8) & 0xFF;
          eight2 = (low2>>8) & 0xFF;
          
          temp16 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          low = ((uint32_t)temp16 << 16) | temp;

          eight1 = (low1>>16) & 0xFF;
          eight2 = (low2>>16) & 0xFF;
          
          temp16 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp = temp16 & 0xFFFF;

          eight1 = (low1>>24) & 0xFF;
          eight2 = (low2>>24) & 0xFF;
          
          temp16 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          high = ((uint32_t)temp16 << 16) | temp;

          eight1 = high1 & 0xFF;
          eight2 = high2 & 0xFF;
          
          temp16 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp = temp16 & 0xFFFF;

          eight1 = (high1>>8) & 0xFF;
          eight2 = (high2>>8) & 0xFF;
          
          temp16 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          quadlow = ((uint32_t)temp16 << 16) | temp;

          eight1 = (high1>>16) & 0xFF;
          eight2 = (high2>>16) & 0xFF;
          
          temp16 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          temp = temp16 & 0xFFFF;

          eight1 = (high1>>24) & 0xFF;
          eight2 = (high2>>24) & 0xFF;
          
          temp16 = (eight1 > eight2) ? (eight1 - eight2) : (eight2 - eight1);
          quadhigh = ((uint32_t)temp16 << 16) | temp;
        }
      }
      else {
        if(!is_long) {
          for(int i = 0; i < 4; i++) {
            eight1 = (low1 >> (8*i)) & 0xFF;
            eight2 = (low2 >> (8*i)) & 0xFF;
            
            temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
            temp |= temp8 << (8*i);
          }

          low = temp;
          temp = 0;

          for(int i = 0; i < 4; i++) {
            eight1 = (high1 >> (8*i)) & 0xFF;
            eight2 = (high2 >> (8*i)) & 0xFF;
            
            temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
            temp |= temp8 << (8*i);
          }

          high = temp;

          if(quad) {
            temp = 0;
            for(int i = 0; i < 4; i++) {
              eight1 = (quadlow1 >> (8*i)) & 0xFF;
              eight2 = (quadlow2 >> (8*i)) & 0xFF;
              
              temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
              temp |= temp8 << (8*i);
            }

            quadlow = temp;
            temp = 0;

            for(int i = 0; i < 2; i++) {
              eight1 = (quadhigh1 >> (8*i)) & 0xFF;
              eight2 = (quadhigh2 >> (8*i)) & 0xFF;
              
              temp8 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
              temp |= temp8 << (8*i);
            }

            quadhigh = temp;
          }
        }
        else {
          eight1 = low1 & 0xFF;
          eight2 = low2 & 0xFF;
          
          temp16 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp = temp16 & 0xFFFF;

          eight1 = (low1>>8) & 0xFF;
          eight2 = (low2>>8) & 0xFF;
          
          temp16 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          low = ((uint32_t)temp16 << 16) | temp;

          eight1 = (low1>>16) & 0xFF;
          eight2 = (low2>>16) & 0xFF;
          
          temp16 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp = temp16 & 0xFFFF;

          eight1 = (low1>>24) & 0xFF;
          eight2 = (low2>>24) & 0xFF;
          
          temp16 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          high = ((uint32_t)temp16 << 16) | temp;

          eight1 = high1 & 0xFF;
          eight2 = high2 & 0xFF;
          
          temp16 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp = temp16 & 0xFFFF;

          eight1 = (high1>>8) & 0xFF;
          eight2 = (high2>>8) & 0xFF;
          
          temp16 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          quadlow = ((uint32_t)temp16 << 16) | temp;

          eight1 = (high1>>16) & 0xFF;
          eight2 = (high2>>16) & 0xFF;
          
          temp16 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          temp = temp16 & 0xFFFF;

          eight1 = (high1>>24) & 0xFF;
          eight2 = (high2>>24) & 0xFF;
          
          temp16 = ((int8_t)eight1 > (int8_t)eight2) ? ((int8_t)eight1 - (int8_t)eight2) : ((int8_t)eight2 - (int8_t)eight1);
          quadhigh = ((uint32_t)temp16 << 16) | temp;
        }
      }
      break;
    case 1: //16-bits
      if(is_unsigned) {
        if(!is_long) {
          for(int i = 0; i < 2; i++) {
            sixteen1 = (low1 >> (16*i)) & 0xFFFF;
            sixteen2 = (low2 >> (16*i)) & 0xFFFF;
            
            temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
            temp |= temp16 << (16*i);
          }

          low = temp;
          temp = 0;

          for(int i = 0; i < 2; i++) {
            sixteen1 = (high1 >> (16*i)) & 0xFFFF;
            sixteen2 = (high2 >> (16*i)) & 0xFFFF;
            
            temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
            temp |= temp16 << (16*i);
          }

          high = temp;

          if(quad) {
            temp = 0;
            for(int i = 0; i < 2; i++) {
              sixteen1 = (quadlow1 >> (16*i)) & 0xFFFF;
              sixteen2 = (quadlow2 >> (16*i)) & 0xFFFF;
              
              temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
              temp |= temp16 << (16*i);
            }

            quadlow = temp;
            temp = 0;

            for(int i = 0; i < 2; i++) {
              sixteen1 = (quadhigh1 >> (16*i)) & 0xFFFF;
              sixteen2 = (quadhigh2 >> (16*i)) & 0xFFFF;
              
              temp16 = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);
              temp |= temp16 << (16*i);
            }

            quadhigh = temp;
          }
        }
        else {
          sixteen1 = low1 & 0xFFFF;
          sixteen2 = low2 & 0xFFFF;
          low = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);

          sixteen1 = (low1>>16) & 0xFFFF;
          sixteen2 = (low2>>16) & 0xFFFF;
          high = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);

          temp64U = ((uint64_t)high << 32) | low;
          prog->st32( arg1 + 16, temp64U);

          sixteen1 = high1 & 0xFFFF;
          sixteen2 = high2 & 0xFFFF;
          quadlow = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);

          sixteen1 = (high1>>16) & 0xFFFF;
          sixteen2 = (high2>>16) & 0xFFFF;
          quadhigh = (sixteen1 > sixteen2) ? (sixteen1 - sixteen2) : (sixteen2 - sixteen1);

          temp64U = ((uint64_t)quadhigh << 32) | quadlow;
          prog->st32( arg1 + 16, temp64U);
        }
      }
      else {
        if(!is_long) {
          for(int i = 0; i < 2; i++) {
            sixteen1 = (low1 >> (16*i)) & 0xFFFF;
            sixteen2 = (low2 >> (16*i)) & 0xFFFF;
            
            temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen1 - (int16_t)sixteen1);
            temp |= temp16 << (16*i);
          }

          low = temp;
          temp = 0;

          for(int i = 0; i < 2; i++) {
            sixteen1 = (high1 >> (16*i)) & 0xFFFF;
            sixteen2 = (high2 >> (16*i)) & 0xFFFF;
            
            temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
            temp |= temp16 << (16*i);
          }

          high = temp;

          if(quad) {
            temp = 0;
            for(int i = 0; i < 2; i++) {
              sixteen1 = (quadlow1 >> (16*i)) & 0xFFFF;
              sixteen2 = (quadlow2 >> (16*i)) & 0xFFFF;
              
              temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
              temp |= temp16 << (16*i);
            }

            quadlow = temp;
            temp = 0;

            for(int i = 0; i < 2; i++) {
              sixteen1 = (quadhigh1 >> (16*i)) & 0xFFFF;
              sixteen2 = (quadhigh2 >> (16*i)) & 0xFFFF;
              
              temp16 = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);
              temp |= temp16 << (16*i);
            }

            quadhigh = temp;
          }
        }
        else {
          sixteen1 = low1 & 0xFFFF;
          sixteen2 = low2 & 0xFFFF;
          low = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);

          sixteen1 = (low1>>16) & 0xFFFF;
          sixteen2 = (low2>>16) & 0xFFFF;
          high = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);

          temp64S = ((uint64_t)high << 32) | low;
          prog->st32( arg1 + 16, temp64S);

          sixteen1 = high1 & 0xFFFF;
          sixteen2 = high2 & 0xFFFF;
          quadlow = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);

          sixteen1 = (high1>>16) & 0xFFFF;
          sixteen2 = (high2>>16) & 0xFFFF;
          quadhigh = ((int16_t)sixteen1 > (int16_t)sixteen2) ? ((int16_t)sixteen1 - (int16_t)sixteen2) : ((int16_t)sixteen2 - (int16_t)sixteen1);

          temp64S = ((uint64_t)quadhigh << 32) | quadlow;
          prog->st32( arg1 + 16, temp64S);
        }
      }
      break;
    case 2: //32-bits
      if(is_unsigned) {
        if(!is_long) {
          low = (low1 > low2) ? (low1 - low2) : (low2 - low1);
          high = (high1 > high2) ? (high1 - high2) : (high2 - high1);

          if(quad) {
            quadlow = (quadlow1 > quadlow2) ? (quadlow1 - quadlow2) : (quadlow2 - quadlow1);
            quadhigh = (quadhigh1 > quadhigh2) ? (quadhigh1 - quadhigh2) : (quadhigh2 - quadhigh1);
          }
        }
        else {
          temp64U = (low1 > low2) ? ((low1 - low2) & 0xFFFFFFFF) : ((low2 - low1) & 0xFFFFFFFF);
          prog->st32( arg1 + 16, temp64U);

          temp64U = (high1 > high2) ? ((high1 - high2) & 0xFFFFFFFF) : ((high2 - high1) & 0xFFFFFFFF);
          prog->st32( arg1 + 24, temp64U);
        }
      }
      else {
        if(!is_long) {
          low = ((int32_t)low1 > (int32_t)low2) ? ((int32_t)low1 - (int32_t)low2) : ((int32_t)low2 - (int32_t)low1);
          high = ((int32_t)high1 > (int32_t)high2) ? ((int32_t)high1 - (int32_t)high2) : ((int32_t)high2 - (int32_t)high1);

          if(quad) {
            quadlow = ((int32_t)quadlow1 > (int32_t)quadlow2) ? ((int32_t)quadlow1 - (int32_t)quadlow2) : ((int32_t)quadlow2 - (int32_t)quadlow1);
            quadhigh = ((int32_t)quadhigh1 > (int32_t)quadhigh2) ? ((int32_t)quadhigh1 - (int32_t)quadhigh2) : ((int32_t)quadhigh2 - (int32_t)quadhigh1);
          }
        }
        else {
          temp64S = ((int32_t)low1 > (int32_t)low2) ? ((int32_t)low1 - (int32_t)low2) : ((int32_t)low2 - (int32_t)low1);
          prog->st32( arg1 + 16, temp64S);

          temp64S = ((int32_t)high1 > (int32_t)high2) ? ((int32_t)high1 - (int32_t)high2) : ((int32_t)high2 - (int32_t)high1);
          prog->st32( arg1 + 24, temp64S);
        }
      }
      break;
  }

  if(!is_long) {
    prog->st32( arg1 + 16, low);
    prog->st32( arg1 + 20, high );

    if(quad) {
      prog->st32( arg1 + 24, quadlow);
      prog->st32( arg1 + 28, quadhigh );
    }
  }

  return 0;
}

uint32_t do_vcnt(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low      = prog->ldu32( arg1+4 ) ;
  uint32_t high     = prog->ldu32( arg1+8 ) ;
  uint32_t quad     = prog->ldu32( arg1+12 ) ;
  uint32_t quadlow;
  uint32_t quadhigh;

  if(quad) {
    quadlow  = prog->ldu32( arg1+36 ) ;
    quadhigh = prog->ldu32( arg1+40 ) ;
  }

  // do VCNT
  low = (low & 0x55555555) + ((low >>  1) & 0x55555555);
  low = (low & 0x33333333) + ((low >>  2) & 0x33333333);
  low = (low & 0x0f0f0f0f) + ((low >>  4) & 0x0f0f0f0f);

  high = (high & 0x55555555) + ((high >>  1) & 0x55555555);
  high = (high & 0x33333333) + ((high >>  2) & 0x33333333);
  high = (high & 0x0f0f0f0f) + ((high >>  4) & 0x0f0f0f0f);

  if(quad) {
    quadlow = (quadlow & 0x55555555) + ((quadlow >>  1) & 0x55555555);
    quadlow = (quadlow & 0x33333333) + ((quadlow >>  2) & 0x33333333);
    quadlow = (quadlow & 0x0f0f0f0f) + ((quadlow >>  4) & 0x0f0f0f0f);

    quadhigh = (quadhigh & 0x55555555) + ((quadhigh >>  1) & 0x55555555);
    quadhigh = (quadhigh & 0x33333333) + ((quadhigh >>  2) & 0x33333333);
    quadhigh = (quadhigh & 0x0f0f0f0f) + ((quadhigh >>  4) & 0x0f0f0f0f);
  }

  prog->st32( arg1 + 16, low) ;
  prog->st32( arg1 + 20, high ) ;

  if(quad) {
    prog->st32( arg1 + 24, quadlow) ;
    prog->st32( arg1 + 28, quadhigh ) ;
  }

  return 0;
}
//**************************//
// vector convert fixed point to floating point
// unsigned, 16 and 32 bit combinations
// sad to see no one comments :-(
// 
// pseudocode in ARMv7:
// A8-582---A8-583 in ARMv7 pdf
//
// fixed to floating point
//**************************//
uint32_t do_vcvtfxfp(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low   = prog->ldu32( arg1+4 ); //lower32 bits of 64 of VM
  uint32_t high  = prog->ldu32( arg1+8 ); //upper32 bits of 64 of Vm
  uint32_t imm   = prog->ldu32( arg1+12 );//32 bits for size data
  uint32_t sf32  = prog->ldu32( arg1+36 );//double or single precision

  uint8_t size = (imm & 0x3); //00=8bit 01=16bit 10=32bit 11=64bit
  uint8_t sf = (sf32 & 0x1); //1=double precision

  uint16_t tmpres16U; //lower bits
  uint16_t tmpres16U2;//upper bits 
  uint32_t tmpres32U; //lower bits
  uint32_t tmpres32U2;//upper bits

  uint16_t frac_bits16 = 0x10-low; //vm =  low 
  uint32_t frac_bits32 = 0x20-(high|low); 

   switch(size) {
   case 1://16 bits
     if(sf){//double precision
       tmpres16U = low/(1<<frac_bits16);
       tmpres16U2 = high/(1<<frac_bits16);
       prog->st32(arg1+16,(tmpres16U2<<16)|tmpres16U);
     }else{//single precision
       tmpres16U = low/(1<<frac_bits16);
       prog->st32(arg1+16,tmpres16U);
     }
    break;
   case 2: //32 bits
     if(sf){//double precision
       tmpres32U = low/(1<<frac_bits32);
       tmpres32U2 = high/(1<<frac_bits32);
       prog->st32(arg1+16,(tmpres32U2<<32)|tmpres32U);
     }else{//single precision
       tmpres32U = low/(1<<frac_bits32);
       prog->st32(arg1+16,tmpres32U);
     }
    break;
  }
  return 0;
}

uint32_t do_vclz(ProgramBase *prog, uint32_t arg1) 
{
  uint32_t low      = prog->ldu32( arg1+4 ) ;
  uint32_t high     = prog->ldu32( arg1+8 ) ;
  uint32_t imm      = prog->ldu32( arg1+12 ) ;

  uint32_t quadlow;
  uint32_t quadhigh;
  uint8_t  size = (imm & 0x3);
  uint8_t  quad = (imm >> 2) & 0x1;

  int n;
  uint8_t  eight; 
  uint16_t sixteen; 

  uint32_t lresult = 0; 
  uint32_t hresult = 0; 
  uint32_t qlresult = 0; 
  uint32_t qhresult = 0;

  if(quad) {
    quadlow  = prog->ldu32( arg1+36 ) ;
    quadhigh = prog->ldu32( arg1+40 ) ;
  }

  switch(size) {
    case 0: //8-bits
      for(int i = 0; i < 4; i++) {
        eight = (low >> (8*i)) & 0xFF;

        for (n = 8; eight; n--)
          eight >>= 1;

        lresult |= (n << (8*i));
      }

      for(int i = 0; i < 4; i++) {
        eight = (high >> (8*i)) & 0xFF;

        for (n = 8; eight; n--)
          eight >>= 1;

        hresult |= (n << (8*i));
      }

      if(quad) {
        for(int i = 0; i < 4; i++) {
          eight = (quadlow >> (8*i)) & 0xFF;

          for (n = 8; eight; n--)
            eight >>= 1;

          qlresult |= (n << (8*i));
        }

        for(int i = 0; i < 4; i++) {
          eight = (quadhigh >> (8*i)) & 0xFF;

          for (n = 8; eight; n--)
            eight >>= 1;

          qhresult |= (n << (8*i));
        }
      }
      break;
    case 1: //16-bits
      for(int i = 0; i < 2; i++) {
        sixteen = (low >> (16*i)) & 0xFFFF;

        for (n = 16; sixteen; n--)
          sixteen >>= 1;

        lresult |= (n << (16*i));
      }

      for(int i = 0; i < 2; i++) {
        sixteen = (high >> (16*i)) & 0xFFFF;

        for (n = 16; sixteen; n--)
          sixteen >>= 1;

        hresult |= (n << (16*i));
      }

      if(quad) {
        for(int i = 0; i < 2; i++) {
          sixteen = (quadlow >> (16*i)) & 0xFFFF;

          for (n = 16; sixteen; n--)
            sixteen >>= 1;

          qlresult |= (n << (16*i));
        }

        for(int i = 0; i < 2; i++) {
          sixteen = (quadhigh >> (16*i)) & 0xFFFF;

          for (n = 16; sixteen; n--)
            sixteen >>= 1;

          qhresult |= (n << (16*i));
        }
      }
      break;
    case 2: //32-bits
      for (n = 32; low; n--)
        low >>= 1;

      lresult = n;

      for (n = 32; high; n--)
        high >>= 1;

      hresult = n;

      if(quad) {
        for (n = 32; quadlow; n--)
          quadlow >>= 1;

        qlresult = n;

        for (n = 32; quadhigh; n--)
          quadhigh>>= 1;

        qhresult = n;
      }
      break;
  }

  prog->st32( arg1 + 16, lresult );
  prog->st32( arg1 + 20, hresult );

  if(quad) {
    prog->st32( arg1 + 24, qlresult );
    prog->st32( arg1 + 28, qhresult );
  }

  return 0;
}

uint32_t do_vcls(ProgramBase *prog, uint32_t arg1) 
{
  uint32_t low      = prog->ldu32( arg1+4 ) ;
  uint32_t high     = prog->ldu32( arg1+8 ) ;
  uint32_t imm      = prog->ldu32( arg1+12 ) ;

  uint32_t quadlow;
  uint32_t quadhigh;
  uint8_t  size = (imm & 0x3);
  uint8_t  quad = (imm >> 2) & 0x1;

  int n;
  uint8_t  eight; 
  uint16_t sixteen; 

  uint32_t lresult = 0; 
  uint32_t hresult = 0; 
  uint32_t qlresult = 0; 
  uint32_t qhresult = 0;

  if(quad) {
    quadlow  = prog->ldu32( arg1+36 ) ;
    quadhigh = prog->ldu32( arg1+40 ) ;
  }

  switch(size) {
    case 0: //8-bits
      for(int i = 0; i < 4; i++) {
        eight = (low >> (8*i)) & 0xFF;
        if((int8_t)eight < 0)
          eight = ~eight;

        for (n = 8; eight; n--)
          eight >>= 1;

        lresult |= ((n-1) << (8*i));
      }

      for(int i = 0; i < 4; i++) {
        eight = (high >> (8*i)) & 0xFF;
        if((int8_t)eight < 0)
          eight = ~eight;

        for (n = 8; eight; n--)
          eight >>= 1;

        hresult |= ((n-1) << (8*i));
      }

      if(quad) {
        for(int i = 0; i < 4; i++) {
          eight = (quadlow >> (8*i)) & 0xFF;
          if((int8_t)eight < 0)
            eight = ~eight;

          for (n = 8; eight; n--)
            eight >>= 1;

          qlresult |= ((n-1) << (8*i));
        }

        for(int i = 0; i < 4; i++) {
          eight = (quadhigh >> (8*i)) & 0xFF;
          if((int8_t)eight < 0)
            eight = ~eight;

          for (n = 8; eight; n--)
            eight >>= 1;

          qhresult |= ((n-1) << (8*i));
        }
      }
      break;
    case 1: //16-bits
      for(int i = 0; i < 2; i++) {
        sixteen = (low >> (16*i)) & 0xFFFF;
        if((int16_t)sixteen < 0)
          sixteen = ~sixteen;

        for (n = 16; sixteen; n--)
          sixteen >>= 1;

        lresult |= ((n-1) << (16*i));
      }

      for(int i = 0; i < 2; i++) {
        sixteen = (high >> (16*i)) & 0xFFFF;
        if((int16_t)sixteen < 0)
          sixteen = ~sixteen;

        for (n = 16; sixteen; n--)
          sixteen >>= 1;

        hresult |= ((n-1) << (16*i));
      }

      if(quad) {
        for(int i = 0; i < 2; i++) {
          sixteen = (quadlow >> (16*i)) & 0xFFFF;
          if((int16_t)sixteen < 0)
            sixteen = ~sixteen;

          for (n = 16; sixteen; n--)
            sixteen >>= 1;

          qlresult |= ((n-1) << (16*i));
        }

        for(int i = 0; i < 2; i++) {
          sixteen = (quadhigh >> (16*i)) & 0xFFFF;
          if((int16_t)sixteen < 0)
            sixteen = ~sixteen;

          for (n = 16; sixteen; n--)
            sixteen >>= 1;

          qhresult |= ((n-1) << (16*i));
        }
      }
      break;
    case 2: //32-bits
      if((int32_t) low < 0)
        low = ~low;

      for (n = 32; low; n--)
        low >>= 1;

      lresult = n-1;

      if((int32_t) high < 0)
        high = ~high;

      for (n = 32; high; n--)
        high >>= 1;

      hresult = n-1;

      if(quad) {
        if((int32_t) quadlow < 0)
          quadlow = ~quadlow;

        for (n = 32; quadlow; n--)
          quadlow >>= 1;

        qlresult = n-1;

        if((int32_t) quadhigh < 0)
          quadhigh = ~quadhigh;

        for (n = 32; quadhigh; n--)
          quadhigh>>= 1;

        qhresult = n-1;
      }
      break;
  }

  prog->st32( arg1 + 16, lresult );
  prog->st32( arg1 + 20, hresult );

  if(quad) {
    prog->st32( arg1 + 24, qlresult );
    prog->st32( arg1 + 28, qhresult );
  }

  return 0;
}

uint32_t do_vqshl(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low      = prog->ldu32( arg1+4 ) ;
  uint32_t high     = prog->ldu32( arg1+8 ) ;
  uint32_t imm      = prog->ldu32( arg1+12 ) ;

  uint32_t low2;
  uint32_t high2;
  uint32_t quadlow;
  uint32_t quadhigh;
  uint32_t quadlow2;
  uint32_t quadhigh2;

  uint8_t size = (imm & 0x3);
  uint8_t quad = (imm >> 2) & 0x1;
  uint8_t is_unsigned = (imm>>3) & 0x1;
  uint8_t shift_amount = (imm>>4) & 0x3F;
  uint8_t qshlu = (imm>>10) & 0x1;
  uint8_t immS = (imm>>11) & 0x1;

  uint8_t tmpres8U, tmpres8U2, tmpres8U3, tmpres8U4;
  int8_t tmpres8S, tmpres8S2, tmpres8S3, tmpres8S4;

  uint16_t tmpres16U;
  int16_t tmpres16S;

  uint32_t lresult;
  uint32_t hresult;
  uint32_t qlresult;
  uint32_t qhresult;

  uint64_t valU;
  int64_t valS;

  int8_t tmp;
  uint8_t sat = 0;
  uint8_t shift;
  bool done = false;

  if(quad) {
    low2        = prog->ldu32( arg1+44 ) ;
    high2       = prog->ldu32( arg1+48 ) ;

    quadlow    = prog->ldu32( arg1+36 ) ;
    quadhigh   = prog->ldu32( arg1+40 ) ;

    quadlow2    = prog->ldu32( arg1+52 ) ;
    quadhigh2   = prog->ldu32( arg1+56 ) ;
  }
  else {
    low2  = prog->ldu32( arg1+28 ) ;
    high2 = prog->ldu32( arg1+32 ) ;
  }


  switch(size) {
    case 0: //8-bit
      if(is_unsigned) {
        if(qshlu) {
          uint8_t src1 = low2 & 0xFF;
          if(src1 & (1 << 7)) { 
            sat = 1; 
            tmpres8U = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)8) { 
              if(src1) { 
                sat = 1; 
                tmpres8U = ~0; 
              } 
              else { 
                tmpres8U = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)8) { 
              tmpres8U = 0; 
            } 
            else if(tmp < 0) { 
              tmpres8U = src1 >> -tmp; 
            } 
            else { 
              tmpres8U = src1 << tmp; 
              if((tmpres8U >> tmp) != src1) { 
                sat = 1; 
                tmpres8U = ~0; 
              } 
            } 
          }

          src1 = (low2>>8) & 0xFF;
          if(src1 & (1 << 7)) { 
            sat = 1; 
            tmpres8U2 = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)8) { 
              if(src1) { 
                sat = 1; 
                tmpres8U2 = ~0; 
              } 
              else { 
                tmpres8U2 = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)8) { 
              tmpres8U2 = 0; 
            } 
            else if(tmp < 0) { 
              tmpres8U2 = src1 >> -tmp; 
            } 
            else { 
              tmpres8U2 = src1 << tmp; 
              if((tmpres8U2 >> tmp) != src1) { 
                sat = 1; 
                tmpres8U2 = ~0; 
              } 
            } 
          }

          src1 = (low2>>16) & 0xFF;
          if(src1 & (1 << 7)) { 
            sat = 1; 
            tmpres8U3 = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)8) { 
              if(src1) { 
                sat = 1; 
                tmpres8U3 = ~0; 
              } 
              else { 
                tmpres8U3 = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)8) { 
              tmpres8U3 = 0; 
            } 
            else if(tmp < 0) { 
              tmpres8U3 = src1 >> -tmp; 
            } 
            else { 
              tmpres8U3 = src1 << tmp; 
              if((tmpres8U3 >> tmp) != src1) { 
                sat = 1; 
                tmpres8U3 = ~0; 
              } 
            } 
          }

          src1 = (low2>>24) & 0xFF;
          if(src1 & (1 << 7)) { 
            sat = 1; 
            tmpres8U4 = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)8) { 
              if(src1) { 
                sat = 1; 
                tmpres8U4 = ~0; 
              } 
              else { 
                tmpres8U4 = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)8) { 
              tmpres8U4 = 0; 
            } 
            else if(tmp < 0) { 
              tmpres8U4 = src1 >> -tmp; 
            } 
            else { 
              tmpres8U4 = src1 << tmp; 
              if((tmpres8U4 >> tmp) != src1) { 
                sat = 1; 
                tmpres8U4 = ~0; 
              } 
            } 
          }

          lresult = ((uint32_t)tmpres8U4 << 24) | ((uint32_t)tmpres8U3 << 16) | 
                    ((uint32_t)tmpres8U2 << 8) | (uint32_t)tmpres8U;

          src1 = high2 & 0xFF;
          if(src1 & (1 << 7)) { 
            sat = 1; 
            tmpres8U = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)8) { 
              if(src1) { 
                sat = 1; 
                tmpres8U = ~0; 
              } 
              else { 
                tmpres8U = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)8) { 
              tmpres8U = 0; 
            } 
            else if(tmp < 0) { 
              tmpres8U = src1 >> -tmp; 
            } 
            else { 
              tmpres8U = src1 << tmp; 
              if((tmpres8U >> tmp) != src1) { 
                sat = 1; 
                tmpres8U = ~0; 
              } 
            } 
          }

          src1 = (high2>>8) & 0xFF;
          if(src1 & (1 << 7)) { 
            sat = 1; 
            tmpres8U2 = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)8) { 
              if(src1) { 
                sat = 1; 
                tmpres8U2 = ~0; 
              } 
              else { 
                tmpres8U2 = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)8) { 
              tmpres8U2 = 0; 
            } 
            else if(tmp < 0) { 
              tmpres8U2 = src1 >> -tmp; 
            } 
            else { 
              tmpres8U2 = src1 << tmp; 
              if((tmpres8U2 >> tmp) != src1) { 
                sat = 1; 
                tmpres8U2 = ~0; 
              } 
            } 
          }

          src1 = (high2>>16) & 0xFF;
          if(src1 & (1 << 7)) { 
            sat = 1; 
            tmpres8U3 = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)8) { 
              if(src1) { 
                sat = 1; 
                tmpres8U3 = ~0; 
              } 
              else { 
                tmpres8U3 = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)8) { 
              tmpres8U3 = 0; 
            } 
            else if(tmp < 0) { 
              tmpres8U3 = src1 >> -tmp; 
            } 
            else { 
              tmpres8U3 = src1 << tmp; 
              if((tmpres8U3 >> tmp) != src1) { 
                sat = 1; 
                tmpres8U3 = ~0; 
              } 
            } 
          }

          src1 = (high2>>24) & 0xFF;
          if(src1 & (1 << 7)) { 
            sat = 1; 
            tmpres8U4 = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)8) { 
              if(src1) { 
                sat = 1; 
                tmpres8U4 = ~0; 
              } 
              else { 
                tmpres8U4 = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)8) { 
              tmpres8U4 = 0; 
            } 
            else if(tmp < 0) { 
              tmpres8U4 = src1 >> -tmp; 
            } 
            else { 
              tmpres8U4 = src1 << tmp; 
              if((tmpres8U4 >> tmp) != src1) { 
                sat = 1; 
                tmpres8U4 = ~0; 
              } 
            } 
          }

          hresult = ((uint32_t)tmpres8U4 << 24) | ((uint32_t)tmpres8U3 << 16) | 
                    ((uint32_t)tmpres8U2 << 8) | (uint32_t)tmpres8U;

          if(quad) {
            src1 = quadlow2 & 0xFF;
            if(src1 & (1 << 7)) { 
              sat = 1; 
              tmpres8U = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)8) { 
                if(src1) { 
                  sat = 1; 
                  tmpres8U = ~0; 
                } 
                else { 
                  tmpres8U = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)8) { 
                tmpres8U = 0; 
              } 
              else if(tmp < 0) { 
                tmpres8U = src1 >> -tmp; 
              } 
              else { 
                tmpres8U = src1 << tmp; 
                if((tmpres8U >> tmp) != src1) { 
                  sat = 1; 
                  tmpres8U = ~0; 
                } 
              } 
            }

            src1 = (quadlow2>>8) & 0xFF;
            if(src1 & (1 << 7)) { 
              sat = 1; 
              tmpres8U2 = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)8) { 
                if(src1) { 
                  sat = 1; 
                  tmpres8U2 = ~0; 
                } 
                else { 
                  tmpres8U2 = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)8) { 
                tmpres8U2 = 0; 
              } 
              else if(tmp < 0) { 
                tmpres8U2 = src1 >> -tmp; 
              } 
              else { 
                tmpres8U2 = src1 << tmp; 
                if((tmpres8U2 >> tmp) != src1) { 
                  sat = 1; 
                  tmpres8U2 = ~0; 
                } 
              } 
            }

            src1 = (quadlow2>>16) & 0xFF;
            if(src1 & (1 << 7)) { 
              sat = 1; 
              tmpres8U3 = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)8) { 
                if(src1) { 
                  sat = 1; 
                  tmpres8U3 = ~0; 
                } 
                else { 
                  tmpres8U3 = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)8) { 
                tmpres8U3 = 0; 
              } 
              else if(tmp < 0) { 
                tmpres8U3 = src1 >> -tmp; 
              } 
              else { 
                tmpres8U3 = src1 << tmp; 
                if((tmpres8U3 >> tmp) != src1) { 
                  sat = 1; 
                  tmpres8U3 = ~0; 
                } 
              } 
            }

            src1 = (quadlow2>>24) & 0xFF;
            if(src1 & (1 << 7)) { 
              sat = 1; 
              tmpres8U4 = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)8) { 
                if(src1) { 
                  sat = 1; 
                  tmpres8U4 = ~0; 
                } 
                else { 
                  tmpres8U4 = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)8) { 
                tmpres8U4 = 0; 
              } 
              else if(tmp < 0) { 
                tmpres8U4 = src1 >> -tmp; 
              } 
              else { 
                tmpres8U4 = src1 << tmp; 
                if((tmpres8U4 >> tmp) != src1) { 
                  sat = 1; 
                  tmpres8U4 = ~0; 
                } 
              } 
            }

            qlresult = ((uint32_t)tmpres8U4 << 24) | ((uint32_t)tmpres8U3 << 16) | 
                      ((uint32_t)tmpres8U2 << 8) | (uint32_t)tmpres8U;

            src1 = quadhigh2 & 0xFF;
            if(src1 & (1 << 7)) { 
              sat = 1; 
              tmpres8U = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)8) { 
                if(src1) { 
                  sat = 1; 
                  tmpres8U = ~0; 
                } 
                else { 
                  tmpres8U = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)8) { 
                tmpres8U = 0; 
              } 
              else if(tmp < 0) { 
                tmpres8U = src1 >> -tmp; 
              } 
              else { 
                tmpres8U = src1 << tmp; 
                if((tmpres8U >> tmp) != src1) { 
                  sat = 1; 
                  tmpres8U = ~0; 
                } 
              } 
            }

            src1 = (quadhigh2>>8) & 0xFF;
            if(src1 & (1 << 7)) { 
              sat = 1; 
              tmpres8U2 = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)8) { 
                if(src1) { 
                  sat = 1; 
                  tmpres8U2 = ~0; 
                } 
                else { 
                  tmpres8U2 = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)8) { 
                tmpres8U2 = 0; 
              } 
              else if(tmp < 0) { 
                tmpres8U2 = src1 >> -tmp; 
              } 
              else { 
                tmpres8U2 = src1 << tmp; 
                if((tmpres8U2 >> tmp) != src1) { 
                  sat = 1; 
                  tmpres8U2 = ~0; 
                } 
              } 
            }

            src1 = (quadhigh2>>16) & 0xFF;
            if(src1 & (1 << 7)) { 
              sat = 1; 
              tmpres8U3 = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)8) { 
                if(src1) { 
                  sat = 1; 
                  tmpres8U3 = ~0; 
                } 
                else { 
                  tmpres8U3 = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)8) { 
                tmpres8U3 = 0; 
              } 
              else if(tmp < 0) { 
                tmpres8U3 = src1 >> -tmp; 
              } 
              else { 
                tmpres8U3 = src1 << tmp; 
                if((tmpres8U3 >> tmp) != src1) { 
                  sat = 1; 
                  tmpres8U3 = ~0; 
                } 
              } 
            }

            src1 = (quadhigh2>>24) & 0xFF;
            if(src1 & (1 << 7)) { 
              sat = 1; 
              tmpres8U4 = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)8) { 
                if(src1) { 
                  sat = 1; 
                  tmpres8U4 = ~0; 
                } 
                else { 
                  tmpres8U4 = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)8) { 
                tmpres8U4 = 0; 
              } 
              else if(tmp < 0) { 
                tmpres8U4 = src1 >> -tmp; 
              } 
              else { 
                tmpres8U4 = src1 << tmp; 
                if((tmpres8U4 >> tmp) != src1) { 
                  sat = 1; 
                  tmpres8U4 = ~0; 
                } 
              } 
            }

            qhresult = ((uint32_t)tmpres8U4 << 24) | ((uint32_t)tmpres8U3 << 16) | 
                      ((uint32_t)tmpres8U2 << 8) | (uint32_t)tmpres8U;
          }
        } //end qshlu
        else {
          tmp = (immS) ? shift_amount : (int8_t)(low & 0xFF);
          if (tmp >= 8) { 
            if (low2 & 0xFF) { 
              sat = 1;
              tmpres8U = ~0; 
            }
            else { 
              tmpres8U = 0; 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8U = 0; 
          }
          else if (tmp < 0) { 
            tmpres8U = (low2 & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8U = (low2 & 0xFF) << tmp; 
            if ((tmpres8U >> tmp) != (uint8_t)(low2 & 0xFF)) { 
              sat = 1;
              tmpres8U = ~0; 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((low>>8) & 0xFF);
          if (tmp >= 8) { 
            if ((low2>>8) & 0xFF) { 
              sat = 1;
              tmpres8U2 = ~0; 
            }
            else { 
              tmpres8U2 = 0; 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8U2 = 0; 
          }
          else if (tmp < 0) { 
            tmpres8U2 = ((low2>>8) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8U2 = ((low2>>8) & 0xFF) << tmp; 
            if ((tmpres8U2 >> tmp) != (uint8_t)((low2>>8) & 0xFF)) { 
              sat = 1;
              tmpres8U2 = ~0; 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((low>>16) & 0xFF);
          if (tmp >= 8) { 
            if ((low2>>16) & 0xFF) { 
              sat = 1;
              tmpres8U3 = ~0; 
            }
            else { 
              tmpres8U3 = 0; 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8U3 = 0; 
          }
          else if (tmp < 0) { 
            tmpres8U3 = ((low2>>16) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8U3 = ((low2>>16) & 0xFF) << tmp; 
            if ((tmpres8U3 >> tmp) != (uint8_t)((low2>>16) & 0xFF)) { 
              sat = 1;
              tmpres8U3 = ~0; 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((low>>24) & 0xFF);
          if (tmp >= 8) { 
            if ((low2>>24) & 0xFF) { 
              sat = 1;
              tmpres8U4 = ~0; 
            }
            else { 
              tmpres8U4 = 0; 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8U4 = 0; 
          }
          else if (tmp < 0) { 
            tmpres8U4 = ((low2>>24) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8U4 = ((low2>>24) & 0xFF) << tmp; 
            if ((tmpres8U4 >> tmp) != (uint8_t)((low2>>24) & 0xFF)) { 
              sat = 1;
              tmpres8U4 = ~0; 
            } 
          }

          lresult = ((uint32_t)tmpres8U4 << 24) | ((uint32_t)tmpres8U3 << 16) | ((uint32_t)tmpres8U2 << 8) | (uint32_t)tmpres8U;

          tmp = (immS) ? shift_amount : (int8_t)(high & 0xFF);
          if (tmp >= 8) { 
            if (high2 & 0xFF) { 
              sat = 1;
              tmpres8U = ~0; 
            }
            else { 
              tmpres8U = 0; 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8U = 0; 
          }
          else if (tmp < 0) { 
            tmpres8U = (high2 & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8U = (high2 & 0xFF) << tmp; 
            if ((tmpres8U >> tmp) != (uint8_t)(high2 & 0xFF)) { 
              sat = 1;
              tmpres8U = ~0; 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((high>>8) & 0xFF);
          if (tmp >= 8) { 
            if ((high2>>8) & 0xFF) { 
              sat = 1;
              tmpres8U2 = ~0; 
            }
            else { 
              tmpres8U2 = 0; 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8U2 = 0; 
          }
          else if (tmp < 0) { 
            tmpres8U2 = ((high2>>8) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8U2 = ((high2>>8) & 0xFF) << tmp; 
            if ((tmpres8U2 >> tmp) != (uint8_t)((high2>>8) & 0xFF)) { 
              sat = 1;
              tmpres8U2 = ~0; 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((high>>16) & 0xFF);
          if (tmp >= 8) { 
            if ((high2>>16) & 0xFF) { 
              sat = 1;
              tmpres8U3 = ~0; 
            }
            else { 
              tmpres8U3 = 0; 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8U3 = 0; 
          }
          else if (tmp < 0) { 
            tmpres8U3 = ((high2>>16) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8U3 = ((high2>>16) & 0xFF) << tmp; 
            if ((tmpres8U3 >> tmp) != (uint8_t)((high2>>16) & 0xFF)) { 
              sat = 1;
              tmpres8U3 = ~0; 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((high>>24) & 0xFF);
          if (tmp >= 8) { 
            if ((high2>>24) & 0xFF) { 
              sat = 1;
              tmpres8U4 = ~0; 
            }
            else { 
              tmpres8U4 = 0; 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8U4 = 0; 
          }
          else if (tmp < 0) { 
            tmpres8U4 = ((high2>>24) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8U4 = ((high2>>24) & 0xFF) << tmp; 
            if ((tmpres8U4 >> tmp) != (uint8_t)((high2>>24) & 0xFF)) { 
              sat = 1;
              tmpres8U4 = ~0; 
            } 
          }

          hresult = ((uint32_t)tmpres8U4 << 24) | ((uint32_t)tmpres8U3 << 16) | ((uint32_t)tmpres8U2 << 8) | (uint32_t)tmpres8U;

          if(quad) {
            tmp = (immS) ? shift_amount : (int8_t)(quadlow & 0xFF);
            if (tmp >= 8) { 
              if (quadlow2 & 0xFF) { 
                sat = 1;
                tmpres8U = ~0; 
              }
              else { 
                tmpres8U = 0; 
              } 
            }
            else if (tmp <= -8) { 
              tmpres8U = 0; 
            }
            else if (tmp < 0) { 
              tmpres8U = (quadlow2 & 0xFF) >> -tmp; 
            }
            else { 
              tmpres8U = (quadlow2 & 0xFF) << tmp; 
              if ((tmpres8U >> tmp) != (uint8_t)(quadlow2 & 0xFF)) { 
                sat = 1;
                tmpres8U = ~0; 
              } 
            }

            tmp = (immS) ? shift_amount : (int8_t)((quadlow>>8) & 0xFF);
            if (tmp >= 8) { 
              if ((quadlow2>>8) & 0xFF) { 
                sat = 1;
                tmpres8U2 = ~0; 
              }
              else { 
                tmpres8U2 = 0; 
              } 
            }
            else if (tmp <= -8) { 
              tmpres8U2 = 0; 
            }
            else if (tmp < 0) { 
              tmpres8U2 = ((quadlow2>>8) & 0xFF) >> -tmp; 
            }
            else { 
              tmpres8U2 = ((quadlow2>>8) & 0xFF) << tmp; 
              if ((tmpres8U2 >> tmp) != (uint8_t)((quadlow2>>8) & 0xFF)) { 
                sat = 1;
                tmpres8U2 = ~0; 
              } 
            }

            tmp = (immS) ? shift_amount : (int8_t)((quadlow>>16) & 0xFF);
            if (tmp >= 8) { 
              if ((quadlow2>>16) & 0xFF) { 
                sat = 1;
                tmpres8U3 = ~0; 
              }
              else { 
                tmpres8U3 = 0; 
              } 
            }
            else if (tmp <= -8) { 
              tmpres8U3 = 0; 
            }
            else if (tmp < 0) { 
              tmpres8U3 = ((quadlow2>>16) & 0xFF) >> -tmp; 
            }
            else { 
              tmpres8U3 = ((quadlow2>>16) & 0xFF) << tmp; 
              if ((tmpres8U3 >> tmp) != (uint8_t)((quadlow2>>16) & 0xFF)) { 
                sat = 1;
                tmpres8U3 = ~0; 
              } 
            }

            tmp = (immS) ? shift_amount : (int8_t)((quadlow>>24) & 0xFF);
            if (tmp >= 8) { 
              if ((quadlow2>>24) & 0xFF) { 
                sat = 1;
                tmpres8U4 = ~0; 
              }
              else { 
                tmpres8U4 = 0; 
              } 
            }
            else if (tmp <= -8) { 
              tmpres8U4 = 0; 
            }
            else if (tmp < 0) { 
              tmpres8U4 = ((quadlow2>>24) & 0xFF) >> -tmp; 
            }
            else { 
              tmpres8U4 = ((quadlow2>>24) & 0xFF) << tmp; 
              if ((tmpres8U4 >> tmp) != (uint8_t)((quadlow2>>24) & 0xFF)) { 
                sat = 1;
                tmpres8U4 = ~0; 
              } 
            }

            qlresult = ((uint32_t)tmpres8U4 << 24) | ((uint32_t)tmpres8U3 << 16) | ((uint32_t)tmpres8U2 << 8) | (uint32_t)tmpres8U;

            tmp = (immS) ? shift_amount : (int8_t)(quadhigh & 0xFF);
            if (tmp >= 8) { 
              if (quadhigh2 & 0xFF) { 
                sat = 1;
                tmpres8U = ~0; 
              }
              else { 
                tmpres8U = 0; 
              } 
            }
            else if (tmp <= -8) { 
              tmpres8U = 0; 
            }
            else if (tmp < 0) { 
              tmpres8U = (quadhigh2 & 0xFF) >> -tmp; 
            }
            else { 
              tmpres8U = (quadhigh2 & 0xFF) << tmp; 
              if ((tmpres8U >> tmp) != (uint8_t)(quadhigh2 & 0xFF)) { 
                sat = 1;
                tmpres8U = ~0; 
              } 
            }

            tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>8) & 0xFF);
            if (tmp >= 8) { 
              if ((quadhigh2>>8) & 0xFF) { 
                sat = 1;
                tmpres8U2 = ~0; 
              }
              else { 
                tmpres8U2 = 0; 
              } 
            }
            else if (tmp <= -8) { 
              tmpres8U2 = 0; 
            }
            else if (tmp < 0) { 
              tmpres8U2 = ((quadhigh2>>8) & 0xFF) >> -tmp; 
            }
            else { 
              tmpres8U2 = ((quadhigh2>>8) & 0xFF) << tmp; 
              if ((tmpres8U2 >> tmp) != (uint8_t)((quadhigh2>>8) & 0xFF)) { 
                sat = 1;
                tmpres8U2 = ~0; 
              } 
            }

            tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>16) & 0xFF);
            if (tmp >= 8) { 
              if ((quadhigh2>>16) & 0xFF) { 
                sat = 1;
                tmpres8U3 = ~0; 
              }
              else { 
                tmpres8U3 = 0; 
              } 
            }
            else if (tmp <= -8) { 
              tmpres8U3 = 0; 
            }
            else if (tmp < 0) { 
              tmpres8U3 = ((quadhigh2>>16) & 0xFF) >> -tmp; 
            }
            else { 
              tmpres8U3 = ((quadhigh2>>16) & 0xFF) << tmp; 
              if ((tmpres8U3 >> tmp) != (uint8_t)((quadhigh2>>16) & 0xFF)) { 
                sat = 1;
                tmpres8U3 = ~0; 
              } 
            }

            tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>24) & 0xFF);
            if (tmp >= 8) { 
              if ((quadhigh2>>24) & 0xFF) { 
                sat = 1;
                tmpres8U4 = ~0; 
              }
              else { 
                tmpres8U4 = 0; 
              } 
            }
            else if (tmp <= -8) { 
              tmpres8U4 = 0; 
            }
            else if (tmp < 0) { 
              tmpres8U4 = ((quadhigh2>>24) & 0xFF) >> -tmp; 
            }
            else { 
              tmpres8U4 = ((quadhigh2>>24) & 0xFF) << tmp; 
              if ((tmpres8U4 >> tmp) != (uint8_t)((quadhigh2>>24) & 0xFF)) { 
                sat = 1;
                tmpres8U4 = ~0; 
              } 
            }

            qhresult = ((uint32_t)tmpres8U4 << 24) | ((uint32_t)tmpres8U3 << 16) | ((uint32_t)tmpres8U2 << 8) | (uint32_t)tmpres8U;
          }
        }
      } //end unsigned
      else { //signed
        tmp = (immS) ? shift_amount : (int8_t)(low & 0xFF); 
        if (tmp >= 8) { 
          if (low2 & 0xFF) { 
            sat = 1;
            tmpres8S = (uint32_t)(1 << (8 - 1)); 
            if ((low2 & 0xFF) > 0) { 
              tmpres8S--; 
            } 
          }
          else { 
            tmpres8S = (low2 & 0xFF); 
          } 
        }
        else if (tmp <= -8) { 
          tmpres8S = (int8_t)(low2 & 0xFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres8S = (int8_t)(low2 & 0xFF) >> -tmp; 
        }
        else { 
          tmpres8S = (low2 & 0xFF) << tmp; 
          if (((int8_t)tmpres8S >> tmp) != (int8_t)(low2 & 0xFF)) { 
            sat = 1;
            tmpres8S = (uint32_t)(1 << (8 - 1)); 
            if ((int8_t)(low2 & 0xFF) > 0) { 
              tmpres8S--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)((low>>8) & 0xFF); 
        if (tmp >= 8) { 
          if ((low2>>8) & 0xFF) { 
            sat = 1;
            tmpres8S2 = (uint32_t)(1 << (8 - 1)); 
            if (((low2>>8) & 0xFF) > 0) { 
              tmpres8S2--; 
            } 
          }
          else { 
            tmpres8S2 = ((low2>>8) & 0xFF); 
          } 
        }
        else if (tmp <= -8) { 
          tmpres8S2 = (int8_t)((low2>>8) & 0xFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres8S2 = (int8_t)((low2>>8) & 0xFF) >> -tmp; 
        }
        else { 
          tmpres8S2 = ((low2>>8) & 0xFF) << tmp; 
          if (((int8_t)tmpres8S2 >> tmp) != (int8_t)((low2>>8) & 0xFF)) { 
            sat = 1;
            tmpres8S2 = (uint32_t)(1 << (8 - 1)); 
            if ((int8_t)((low2>>8) & 0xFF) > 0) { 
              tmpres8S2--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)((low>>16) & 0xFF); 
        if (tmp >= 8) { 
          if ((low2>>8) & 0xFF) { 
            sat = 1;
            tmpres8S3 = (uint32_t)(1 << (8 - 1)); 
            if (((low2>>8) & 0xFF) > 0) { 
              tmpres8S3--; 
            } 
          }
          else { 
            tmpres8S3 = ((low2>>16) & 0xFF); 
          } 
        }
        else if (tmp <= -8) { 
          tmpres8S3 = (int8_t)((low2>>16) & 0xFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres8S3 = (int8_t)((low2>>16) & 0xFF) >> -tmp; 
        }
        else { 
          tmpres8S3 = ((low2>>16) & 0xFF) << tmp; 
          if (((int8_t)tmpres8S3 >> tmp) != (int8_t)((low2>>16) & 0xFF)) { 
            sat = 1;
            tmpres8S3 = (uint32_t)(1 << (8 - 1)); 
            if ((int8_t)((low2>>16) & 0xFF) > 0) { 
              tmpres8S3--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)((low>>24) & 0xFF); 
        if (tmp >= 8) { 
          if ((low2>>24) & 0xFF) { 
            sat = 1;
            tmpres8S4 = (uint32_t)(1 << (8 - 1)); 
            if (((low2>>24) & 0xFF) > 0) { 
              tmpres8S4--; 
            } 
          }
          else { 
            tmpres8S4 = ((low2>>24) & 0xFF); 
          } 
        }
        else if (tmp <= -8) { 
          tmpres8S4 = (int8_t)((low2>>24) & 0xFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres8S4 = (int8_t)((low2>>24) & 0xFF) >> -tmp; 
        }
        else { 
          tmpres8S4 = ((low2>>24) & 0xFF) << tmp; 
          if (((int8_t)tmpres8S4 >> tmp) != (int8_t)((low2>>24) & 0xFF)) { 
            sat = 1;
            tmpres8S4 = (uint32_t)(1 << (8 - 1)); 
            if ((int8_t)((low2>>24) & 0xFF) > 0) { 
              tmpres8S4--; 
            } 
          } 
        }

        lresult = ((uint32_t)tmpres8S4 << 24) | ((uint32_t)tmpres8S3 << 16) | ((uint32_t)tmpres8S2 << 8) | tmpres8S;

        tmp = (immS) ? shift_amount : (int8_t)(high & 0xFF); 
        if (tmp >= 8) { 
          if (high2 & 0xFF) { 
            sat = 1;
            tmpres8S = (uint32_t)(1 << (8 - 1)); 
            if ((high2 & 0xFF) > 0) { 
              tmpres8S--; 
            } 
          }
          else { 
            tmpres8S = (high2 & 0xFF); 
          } 
        }
        else if (tmp <= -8) { 
          tmpres8S = (int8_t)(high2 & 0xFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres8S = (int8_t)(high2 & 0xFF) >> -tmp; 
        }
        else { 
          tmpres8S = (high2 & 0xFF) << tmp; 
          if (((int8_t)tmpres8S >> tmp) != (int8_t)(high2 & 0xFF)) { 
            sat = 1;
            tmpres8S = (uint32_t)(1 << (8 - 1)); 
            if ((int8_t)(high2 & 0xFF) > 0) { 
              tmpres8S--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)((high>>8) & 0xFF); 
        if (tmp >= 8) { 
          if ((high2>>8) & 0xFF) { 
            sat = 1;
            tmpres8S2 = (uint32_t)(1 << (8 - 1)); 
            if (((high2>>8) & 0xFF) > 0) { 
              tmpres8S2--; 
            } 
          }
          else { 
            tmpres8S2 = ((high2>>8) & 0xFF); 
          } 
        }
        else if (tmp <= -8) { 
          tmpres8S2 = (int8_t)((high2>>8) & 0xFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres8S2 = (int8_t)((high2>>8) & 0xFF) >> -tmp; 
        }
        else { 
          tmpres8S2 = ((high2>>8) & 0xFF) << tmp; 
          if (((int8_t)tmpres8S2 >> tmp) != (int8_t)((high2>>8) & 0xFF)) { 
            sat = 1;
            tmpres8S2 = (uint32_t)(1 << (8 - 1)); 
            if ((int8_t)((high2>>8) & 0xFF) > 0) { 
              tmpres8S2--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)((high>>16) & 0xFF); 
        if (tmp >= 8) { 
          if ((high2>>16) & 0xFF) { 
            sat = 1;
            tmpres8S3 = (uint32_t)(1 << (8 - 1)); 
            if (((high2>>16) & 0xFF) > 0) { 
              tmpres8S3--; 
            } 
          }
          else { 
            tmpres8S3 = ((high2>>16) & 0xFF); 
          } 
        }
        else if (tmp <= -8) { 
          tmpres8S3 = (int8_t)((high2>>16) & 0xFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres8S3 = (int8_t)((high2>>16) & 0xFF) >> -tmp; 
        }
        else { 
          tmpres8S3 = ((high2>>16) & 0xFF) << tmp; 
          if (((int8_t)tmpres8S3 >> tmp) != (int8_t)((high2>>16) & 0xFF)) { 
            sat = 1;
            tmpres8S3 = (uint32_t)(1 << (8 - 1)); 
            if ((int8_t)((high2>>16) & 0xFF) > 0) { 
              tmpres8S3--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)((high>>24) & 0xFF); 
        if (tmp >= 8) { 
          if ((high2>>24) & 0xFF) { 
            sat = 1;
            tmpres8S4 = (uint32_t)(1 << (8 - 1)); 
            if (((high2>>24) & 0xFF) > 0) { 
              tmpres8S4--; 
            } 
          }
          else { 
            tmpres8S4 = ((high2>>24) & 0xFF); 
          } 
        }
        else if (tmp <= -8) { 
          tmpres8S4 = (int8_t)((high2>>24) & 0xFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres8S4 = (int8_t)((high2>>24) & 0xFF) >> -tmp; 
        }
        else { 
          tmpres8S4 = ((high2>>24) & 0xFF) << tmp; 
          if (((int8_t)tmpres8S4 >> tmp) != (int8_t)((high2>>24) & 0xFF)) { 
            sat = 1;
            tmpres8S4 = (uint32_t)(1 << (8 - 1)); 
            if ((int8_t)((high2>>24) & 0xFF) > 0) { 
              tmpres8S4--; 
            } 
          } 
        }

        hresult = ((uint32_t)tmpres8S4 << 24) | ((uint32_t)tmpres8S3 << 16) | ((uint32_t)tmpres8S2 << 8) | tmpres8S;

        if(quad) {
          tmp = (immS) ? shift_amount : (int8_t)(quadlow & 0xFF); 
          if (tmp >= 8) { 
            if (quadlow2 & 0xFF) { 
              sat = 1;
              tmpres8S = (uint32_t)(1 << (8 - 1)); 
              if ((quadlow2 & 0xFF) > 0) { 
                tmpres8S--; 
              } 
            }
            else { 
              tmpres8S = (quadlow2 & 0xFF); 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8S = (int8_t)(quadlow2 & 0xFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres8S = (int8_t)(quadlow2 & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8S = (quadlow2 & 0xFF) << tmp; 
            if (((int8_t)tmpres8S >> tmp) != (int8_t)(quadlow2 & 0xFF)) { 
              sat = 1;
              tmpres8S = (uint32_t)(1 << (8 - 1)); 
              if ((int8_t)(quadlow2 & 0xFF) > 0) { 
                tmpres8S--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadlow>>8) & 0xFF); 
          if (tmp >= 8) { 
            if ((quadlow2>>8) & 0xFF) { 
              sat = 1;
              tmpres8S2 = (uint32_t)(1 << (8 - 1)); 
              if (((quadlow2>>8) & 0xFF) > 0) { 
                tmpres8S2--; 
              } 
            }
            else { 
              tmpres8S2 = ((quadlow2>>8) & 0xFF); 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8S2 = (int8_t)((quadlow2>>8) & 0xFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres8S2 = (int8_t)((quadlow2>>8) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8S2 = ((quadlow2>>8) & 0xFF) << tmp; 
            if (((int8_t)tmpres8S2 >> tmp) != (int8_t)((quadlow2>>8) & 0xFF)) { 
              sat = 1;
              tmpres8S2 = (uint32_t)(1 << (8 - 1)); 
              if ((int8_t)((quadlow2>>8) & 0xFF) > 0) { 
                tmpres8S2--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadlow>>16) & 0xFF); 
          if (tmp >= 8) { 
            if ((quadlow2>>16) & 0xFF) { 
              sat = 1;
              tmpres8S3 = (uint32_t)(1 << (8 - 1)); 
              if (((quadlow2>>16) & 0xFF) > 0) { 
                tmpres8S3--; 
              } 
            }
            else { 
              tmpres8S3 = ((quadlow2>>16) & 0xFF); 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8S3 = (int8_t)((quadlow2>>16) & 0xFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres8S3 = (int8_t)((quadlow2>>16) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8S3 = ((quadlow2>>16) & 0xFF) << tmp; 
            if (((int8_t)tmpres8S3 >> tmp) != (int8_t)((quadlow2>>16) & 0xFF)) { 
              sat = 1;
              tmpres8S3 = (uint32_t)(1 << (8 - 1)); 
              if ((int8_t)((quadlow2>>16) & 0xFF) > 0) { 
                tmpres8S3--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadlow>>24) & 0xFF); 
          if (tmp >= 8) { 
            if ((quadlow2>>24) & 0xFF) { 
              sat = 1;
              tmpres8S4 = (uint32_t)(1 << (8 - 1)); 
              if (((quadlow2>>24) & 0xFF) > 0) { 
                tmpres8S4--; 
              } 
            }
            else { 
              tmpres8S4 = ((quadlow2>>24) & 0xFF); 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8S4 = (int8_t)((quadlow2>>24) & 0xFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres8S4 = (int8_t)((quadlow2>>24) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8S4 = ((quadlow2>>24) & 0xFF) << tmp; 
            if (((int8_t)tmpres8S4 >> tmp) != (int8_t)((quadlow2>>24) & 0xFF)) { 
              sat = 1;
              tmpres8S4 = (uint32_t)(1 << (8 - 1)); 
              if ((int8_t)((quadlow2>>24) & 0xFF) > 0) { 
                tmpres8S4--; 
              } 
            } 
          }

          qlresult = ((uint32_t)tmpres8S4 << 24) | ((uint32_t)tmpres8S3 << 16) | ((uint32_t)tmpres8S2 << 8) | tmpres8S;

          tmp = (immS) ? shift_amount : (int8_t)(quadhigh & 0xFF); 
          if (tmp >= 8) { 
            if (quadhigh2 & 0xFF) { 
              sat = 1;
              tmpres8S = (uint32_t)(1 << (8 - 1)); 
              if ((quadhigh2 & 0xFF) > 0) { 
                tmpres8S--; 
              } 
            }
            else { 
              tmpres8S = (quadhigh2 & 0xFF); 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8S = (int8_t)(quadhigh2 & 0xFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres8S = (int8_t)(quadhigh2 & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8S = (quadhigh2 & 0xFF) << tmp; 
            if (((int8_t)tmpres8S >> tmp) != (int8_t)(quadhigh2 & 0xFF)) { 
              sat = 1;
              tmpres8S = (uint32_t)(1 << (8 - 1)); 
              if ((int8_t)(quadhigh2 & 0xFF) > 0) { 
                tmpres8S--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>8) & 0xFF); 
          if (tmp >= 8) { 
            if ((quadhigh2>>8) & 0xFF) { 
              sat = 1;
              tmpres8S2 = (uint32_t)(1 << (8 - 1)); 
              if (((quadhigh2>>8) & 0xFF) > 0) { 
                tmpres8S2--; 
              } 
            }
            else { 
              tmpres8S2 = ((quadhigh2>>8) & 0xFF); 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8S2 = (int8_t)((quadhigh2>>8) & 0xFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres8S2 = (int8_t)((quadhigh2>>8) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8S2 = ((quadhigh2>>8) & 0xFF) << tmp; 
            if (((int8_t)tmpres8S2 >> tmp) != (int8_t)((quadhigh2>>8) & 0xFF)) { 
              sat = 1;
              tmpres8S2 = (uint32_t)(1 << (8 - 1)); 
              if ((int8_t)((quadhigh2>>8) & 0xFF) > 0) { 
                tmpres8S2--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>16) & 0xFF); 
          if (tmp >= 8) { 
            if ((quadhigh2>>16) & 0xFF) { 
              sat = 1;
              tmpres8S3 = (uint32_t)(1 << (8 - 1)); 
              if (((quadhigh2>>16) & 0xFF) > 0) { 
                tmpres8S3--; 
              } 
            }
            else { 
              tmpres8S3 = ((quadhigh2>>16) & 0xFF); 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8S3 = (int8_t)((quadhigh2>>16) & 0xFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres8S3 = (int8_t)((quadhigh2>>16) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8S3 = ((quadhigh2>>16) & 0xFF) << tmp; 
            if (((int8_t)tmpres8S3 >> tmp) != (int8_t)((quadhigh2>>16) & 0xFF)) { 
              sat = 1;
              tmpres8S3 = (uint32_t)(1 << (8 - 1)); 
              if ((int8_t)((quadhigh2>>16) & 0xFF) > 0) { 
                tmpres8S3--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>24) & 0xFF); 
          if (tmp >= 8) { 
            if ((quadhigh2>>24) & 0xFF) { 
              sat = 1;
              tmpres8S4 = (uint32_t)(1 << (8 - 1)); 
              if (((quadhigh2>>24) & 0xFF) > 0) { 
                tmpres8S4--; 
              } 
            }
            else { 
              tmpres8S4 = ((quadhigh2>>24) & 0xFF); 
            } 
          }
          else if (tmp <= -8) { 
            tmpres8S4 = (int8_t)((quadhigh2>>24) & 0xFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres8S4 = (int8_t)((quadhigh2>>24) & 0xFF) >> -tmp; 
          }
          else { 
            tmpres8S4 = ((quadhigh2>>24) & 0xFF) << tmp; 
            if (((int8_t)tmpres8S4 >> tmp) != (int8_t)((quadhigh2>>24) & 0xFF)) { 
              sat = 1;
              tmpres8S4 = (uint32_t)(1 << (8 - 1)); 
              if ((int8_t)((quadhigh2>>24) & 0xFF) > 0) { 
                tmpres8S4--; 
              } 
            } 
          }

          qhresult = ((uint32_t)tmpres8S4 << 24) | ((uint32_t)tmpres8S3 << 16) | ((uint32_t)tmpres8S2 << 8) | tmpres8S;
        }
      } //end signed
      break;
    case 1: //16-bit
      if(is_unsigned) {
        if(qshlu) {
          uint16_t src1 = low2 & 0xFFFF;
          if(src1 & (1 << 15)) { 
            sat = 1; 
            tmpres16U = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)16) { 
              if(src1) { 
                sat = 1; 
                tmpres16U = ~0; 
              }
              else { 
                tmpres16U = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)16) { 
              tmpres16U = 0; 
            } 
            else if(tmp < 0) { 
              tmpres16U = src1 >> -tmp; 
            } 
            else { 
              tmpres16U = src1 << tmp; 
              if ((tmpres16U >> tmp) != src1) { 
                sat = 1; 
                tmpres16U = ~0; 
              } 
            } 
          }

          src1 = (low2>>16) & 0xFFFF;
          if(src1 & (1 << 15)) { 
            sat = 1; 
            lresult = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)16) { 
              if(src1) { 
                sat = 1; 
                lresult = ~0; 
              }
              else { 
                lresult = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)16) { 
              lresult = 0; 
            } 
            else if(tmp < 0) { 
              lresult = src1 >> -tmp; 
            } 
            else { 
              lresult = src1 << tmp; 
              if ((lresult >> tmp) != src1) { 
                sat = 1; 
                lresult = ~0; 
              } 
            } 
          }

          lresult = lresult & 0xFFFF;
          lresult = ((uint32_t)lresult << 16) | tmpres16U;

          src1 = high2 & 0xFFFF;
          if(src1 & (1 << 15)) { 
            sat = 1; 
            tmpres16U = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)16) { 
              if(src1) { 
                sat = 1; 
                tmpres16U = ~0; 
              }
              else { 
                tmpres16U = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)16) { 
              tmpres16U = 0; 
            } 
            else if(tmp < 0) { 
              tmpres16U = src1 >> -tmp; 
            } 
            else { 
              tmpres16U = src1 << tmp; 
              if ((tmpres16U >> tmp) != src1) { 
                sat = 1; 
                tmpres16U = ~0; 
              } 
            } 
          }

          src1 = (high2>>16) & 0xFFFF;
          if(src1 & (1 << 15)) { 
            sat = 1; 
            hresult = 0; 
          } 
          else { 
            int8_t tmp; 
            tmp = (int8_t)shift_amount; 
            if(tmp >= (ssize_t)16) { 
              if(src1) { 
                sat = 1; 
                hresult = ~0; 
              }
              else { 
                hresult = 0; 
              } 
            } 
            else if(tmp <= -(ssize_t)16) { 
              hresult = 0; 
            } 
            else if(tmp < 0) { 
              hresult = src1 >> -tmp; 
            } 
            else { 
              hresult = src1 << tmp; 
              if ((hresult >> tmp) != src1) { 
                sat = 1; 
                hresult = ~0; 
              } 
            } 
          }

          hresult = hresult & 0xFFFF;
          hresult = ((uint32_t)hresult << 16) | tmpres16U;

          if(quad) {
            src1 = quadlow2 & 0xFFFF;
            if(src1 & (1 << 15)) { 
              sat = 1; 
              tmpres16U = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)16) { 
                if(src1) { 
                  sat = 1; 
                  tmpres16U = ~0; 
                }
                else { 
                  tmpres16U = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)16) { 
                tmpres16U = 0; 
              } 
              else if(tmp < 0) { 
                tmpres16U = src1 >> -tmp; 
              } 
              else { 
                tmpres16U = src1 << tmp; 
                if ((tmpres16U >> tmp) != src1) { 
                  sat = 1; 
                  tmpres16U = ~0; 
                } 
              } 
            }

            src1 = (quadlow2>>16) & 0xFFFF;
            if(src1 & (1 << 15)) { 
              sat = 1; 
              qlresult = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)16) { 
                if(src1) { 
                  sat = 1; 
                  qlresult = ~0; 
                }
                else { 
                  qlresult = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)16) { 
                qlresult = 0; 
              } 
              else if(tmp < 0) { 
                qlresult = src1 >> -tmp; 
              } 
              else { 
                qlresult = src1 << tmp; 
                if ((qlresult >> tmp) != src1) { 
                  sat = 1; 
                  qlresult = ~0; 
                } 
              } 
            }

            qlresult = qlresult & 0xFFFF;
            qlresult = ((uint32_t)qlresult << 16) | tmpres16U;

            src1 = quadhigh2 & 0xFFFF;
            if(src1 & (1 << 15)) { 
              sat = 1; 
              tmpres16U = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)16) { 
                if(src1) { 
                  sat = 1; 
                  tmpres16U = ~0; 
                }
                else { 
                  tmpres16U = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)16) { 
                tmpres16U = 0; 
              } 
              else if(tmp < 0) { 
                tmpres16U = src1 >> -tmp; 
              } 
              else { 
                tmpres16U = src1 << tmp; 
                if ((tmpres16U >> tmp) != src1) { 
                  sat = 1; 
                  tmpres16U = ~0; 
                } 
              } 
            }

            src1 = (quadhigh2>>16) & 0xFFFF;
            if(src1 & (1 << 15)) { 
              sat = 1; 
              qhresult = 0; 
            } 
            else { 
              int8_t tmp; 
              tmp = (int8_t)shift_amount; 
              if(tmp >= (ssize_t)16) { 
                if(src1) { 
                  sat = 1; 
                  qhresult = ~0; 
                }
                else { 
                  qhresult = 0; 
                } 
              } 
              else if(tmp <= -(ssize_t)16) { 
                qhresult = 0; 
              } 
              else if(tmp < 0) { 
                qhresult = src1 >> -tmp; 
              } 
              else { 
                qhresult = src1 << tmp; 
                if ((qhresult >> tmp) != src1) { 
                  sat = 1; 
                  qhresult = ~0; 
                } 
              } 
            }

            qhresult = qhresult & 0xFFFF;
            qhresult = ((uint32_t)qhresult << 16) | tmpres16U;
          }
        }
        else {
          tmp = (immS) ? shift_amount : (int8_t)(low & 0xFFFF);
          if (tmp >= 16) { 
            if (low2 & 0xFFFF) { 
              sat = 1;
              tmpres16U = ~0; 
            }
            else { 
              tmpres16U = 0; 
            } 
          }
          else if (tmp <= -16) { 
            tmpres16U = 0; 
          }
          else if (tmp < 0) { 
            tmpres16U = (low2 & 0xFFFF) >> -tmp; 
          }
          else { 
            tmpres16U = (low2 & 0xFFFF) << tmp; 
            if ((tmpres16U >> tmp) != (uint16_t)(low2 & 0xFFFF)) { 
              sat = 1;
              tmpres16U = ~0; 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((low>>16) & 0xFFFF);
          if (tmp >= 16) { 
            if ((low2>>16) & 0xFFFF) { 
              sat = 1;
              lresult = ~0; 
            }
            else { 
              lresult = 0; 
            } 
          }
          else if (tmp <= -16) { 
            lresult = 0; 
          }
          else if (tmp < 0) { 
            lresult = ((low2>>16) & 0xFFFF) >> -tmp; 
          }
          else { 
            lresult = ((low2>>16) & 0xFFFF) << tmp; 
            if ((lresult >> tmp) != (uint16_t)((low2>>16) & 0xFFFF)) { 
              sat = 1;
              lresult = ~0; 
            } 
          }

          lresult = lresult & 0xFFFF;
          lresult = ((uint32_t)lresult << 16) | tmpres16U;

          tmp = (immS) ? shift_amount : (int8_t)(quadhigh & 0xFFFF);
          if (tmp >= 16) { 
            if (quadhigh2 & 0xFFFF) { 
              sat = 1;
              tmpres16U = ~0; 
            }
            else { 
              tmpres16U = 0; 
            } 
          }
          else if (tmp <= -16) { 
            tmpres16U = 0; 
          }
          else if (tmp < 0) { 
            tmpres16U = (quadhigh2 & 0xFFFF) >> -tmp; 
          }
          else { 
            tmpres16U = (quadhigh2 & 0xFFFF) << tmp; 
            if ((tmpres16U >> tmp) != (uint16_t)(quadhigh2 & 0xFFFF)) { 
              sat = 1;
              tmpres16U = ~0; 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>16) & 0xFFFF);
          if (tmp >= 16) { 
            if ((quadhigh2>>16) & 0xFFFF) { 
              sat = 1;
              hresult = ~0; 
            }
            else { 
              hresult = 0; 
            } 
          }
          else if (tmp <= -16) { 
            hresult = 0; 
          }
          else if (tmp < 0) { 
            hresult = ((quadhigh2>>16) & 0xFFFF) >> -tmp; 
          }
          else { 
            hresult = ((quadhigh2>>16) & 0xFFFF) << tmp; 
            if ((hresult >> tmp) != (uint16_t)((quadhigh2>>16) & 0xFFFF)) { 
              sat = 1;
              hresult = ~0; 
            } 
          }

          hresult = hresult & 0xFFFF;
          hresult = ((uint32_t)hresult << 16) | tmpres16U;

          if(quad) {
            tmp = (immS) ? shift_amount : (int8_t)(quadlow & 0xFFFF);
            if (tmp >= 16) { 
              if (quadlow2 & 0xFFFF) { 
                sat = 1;
                tmpres16U = ~0; 
              }
              else { 
                tmpres16U = 0; 
              } 
            }
            else if (tmp <= -16) { 
              tmpres16U = 0; 
            }
            else if (tmp < 0) { 
              tmpres16U = (quadlow2 & 0xFFFF) >> -tmp; 
            }
            else { 
              tmpres16U = (quadlow2 & 0xFFFF) << tmp; 
              if ((tmpres16U >> tmp) != (uint16_t)(quadlow2 & 0xFFFF)) { 
                sat = 1;
                tmpres16U = ~0; 
              } 
            }

            tmp = (immS) ? shift_amount : (int8_t)((quadlow>>16) & 0xFFFF);
            if (tmp >= 16) { 
              if ((quadlow2>>16) & 0xFFFF) { 
                sat = 1;
                qlresult = ~0; 
              }
              else { 
                qlresult = 0; 
              } 
            }
            else if (tmp <= -16) { 
              qlresult = 0; 
            }
            else if (tmp < 0) { 
              qlresult = ((quadlow2>>16) & 0xFFFF) >> -tmp; 
            }
            else { 
              qlresult = ((quadlow2>>16) & 0xFFFF) << tmp; 
              if ((qlresult >> tmp) != (uint16_t)((quadlow2>>16) & 0xFFFF)) { 
                sat = 1;
                qlresult = ~0; 
              } 
            }

            qlresult = qlresult & 0xFFFF;
            qlresult = ((uint32_t)qlresult << 16) | tmpres16U;

            tmp = (immS) ? shift_amount : (int8_t)(quadhigh & 0xFFFF);
            if (tmp >= 16) { 
              if (quadhigh2 & 0xFFFF) { 
                sat = 1;
                tmpres16U = ~0; 
              }
              else { 
                tmpres16U = 0; 
              } 
            }
            else if (tmp <= -16) { 
              tmpres16U = 0; 
            }
            else if (tmp < 0) { 
              tmpres16U = (quadhigh2 & 0xFFFF) >> -tmp; 
            }
            else { 
              tmpres16U = (quadhigh2 & 0xFFFF) << tmp; 
              if ((tmpres16U >> tmp) != (uint16_t)(quadhigh2 & 0xFFFF)) { 
                sat = 1;
                tmpres16U = ~0; 
              } 
            }

            tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>16) & 0xFFFF);
            if (tmp >= 16) { 
              if ((quadhigh2>>16) & 0xFFFF) { 
                sat = 1;
                qhresult = ~0; 
              }
              else { 
                qhresult = 0; 
              } 
            }
            else if (tmp <= -16) { 
              qhresult = 0; 
            }
            else if (tmp < 0) { 
              qhresult = ((quadhigh2>>16) & 0xFFFF) >> -tmp; 
            }
            else { 
              qhresult = ((quadhigh2>>16) & 0xFFFF) << tmp; 
              if ((qhresult >> tmp) != (uint16_t)((quadhigh2>>16) & 0xFFFF)) { 
                sat = 1;
                qhresult = ~0; 
              } 
            }

            qhresult = qhresult & 0xFFFF;
            qhresult = ((uint32_t)qhresult << 16) | tmpres16U;
          }
        }
      } //end unsigned
      else { //signed
        tmp = (immS) ? shift_amount : (int8_t)(low & 0xFFFF); 
        if (tmp >= 16) { 
          if (low2 & 0xFFFF) { 
            sat = 1;
            tmpres16S = (uint32_t)(1 << (16 - 1)); 
            if ((low2 & 0xFFFF) > 0) { 
              tmpres16S--; 
            } 
          }
          else { 
            tmpres16S = (low2 & 0xFFFF); 
          } 
        }
        else if (tmp <= -16) { 
          tmpres16S = (int16_t)(low2 & 0xFFFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres16S = (int16_t)(low2 & 0xFFFF) >> -tmp; 
        }
        else { 
          tmpres16S = (low2 & 0xFFFF) << tmp; 
          if (((int16_t)tmpres16S >> tmp) != (int16_t)(low2 & 0xFFFF)) { 
            sat = 1;
            tmpres16S = (uint32_t)(1 << (16 - 1)); 
            if ((int16_t)(low2 & 0xFFFF) > 0) { 
              tmpres16S--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)((low>>16) & 0xFFFF); 
        if (tmp >= 16) { 
          if ((low2>>16) & 0xFFFF) { 
            sat = 1;
            lresult = (uint32_t)(1 << (16 - 1)); 
            if (((low2>>16) & 0xFFFF) > 0) { 
              lresult--; 
            } 
          }
          else { 
            lresult = ((low2>>16) & 0xFFFF); 
          } 
        }
        else if (tmp <= -16) { 
          lresult = (int16_t)((low2>>16) & 0xFFFF) >> 15; 
        } 
        else if (tmp < 0) { 
          lresult = (int16_t)((low2>>16) & 0xFFFF) >> -tmp; 
        }
        else { 
          lresult = ((low2>>16) & 0xFFFF) << tmp; 
          if (((int16_t)lresult >> tmp) != (int16_t)((low2>>16) & 0xFFFF)) { 
            sat = 1;
            lresult = (uint32_t)(1 << (16 - 1)); 
            if ((int16_t)((low2>>16) & 0xFFFF) > 0) { 
              lresult--; 
            } 
          } 
        }

        lresult = lresult & 0xFFFF;
        lresult = ((uint32_t)lresult << 16) | tmpres16S;

        tmp = (immS) ? shift_amount : (int8_t)(high & 0xFFFF); 
        if (tmp >= 16) { 
          if (high2 & 0xFFFF) { 
            sat = 1;
            tmpres16S = (uint32_t)(1 << (16 - 1)); 
            if ((high2 & 0xFFFF) > 0) { 
              tmpres16S--; 
            } 
          }
          else { 
            tmpres16S = (high2 & 0xFFFF); 
          } 
        }
        else if (tmp <= -16) { 
          tmpres16S = (int16_t)(high2 & 0xFFFF) >> 15; 
        } 
        else if (tmp < 0) { 
          tmpres16S = (int16_t)(high2 & 0xFFFF) >> -tmp; 
        }
        else { 
          tmpres16S = (high2 & 0xFFFF) << tmp; 
          if (((int16_t)tmpres16S >> tmp) != (int16_t)(high2 & 0xFFFF)) { 
            sat = 1;
            tmpres16S = (uint32_t)(1 << (16 - 1)); 
            if ((int16_t)(high2 & 0xFFFF) > 0) { 
              tmpres16S--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)((high>>16) & 0xFFFF); 
        if (tmp >= 16) { 
          if ((high2>>16) & 0xFFFF) { 
            sat = 1;
            hresult = (uint32_t)(1 << (16 - 1)); 
            if (((high2>>16) & 0xFFFF) > 0) { 
              hresult--; 
            } 
          }
          else { 
            hresult = ((high2>>16) & 0xFFFF); 
          } 
        }
        else if (tmp <= -16) { 
          hresult = (int16_t)((high2>>16) & 0xFFFF) >> 15; 
        } 
        else if (tmp < 0) { 
          hresult = (int16_t)((high2>>16) & 0xFFFF) >> -tmp; 
        }
        else { 
          hresult = ((high2>>16) & 0xFFFF) << tmp; 
          if (((int16_t)hresult >> tmp) != (int16_t)((high2>>16) & 0xFFFF)) { 
            sat = 1;
            hresult = (uint32_t)(1 << (16 - 1)); 
            if ((int16_t)((high2>>16) & 0xFFFF) > 0) { 
              hresult--; 
            } 
          } 
        }

        hresult = hresult & 0xFFFF;
        hresult = ((uint32_t)hresult << 16) | tmpres16S;

        if(quad) {
          tmp = (immS) ? shift_amount : (int8_t)(quadlow & 0xFFFF); 
          if (tmp >= 16) { 
            if (quadlow2 & 0xFFFF) { 
              sat = 1;
              tmpres16S = (uint32_t)(1 << (16 - 1)); 
              if ((quadlow2 & 0xFFFF) > 0) { 
                tmpres16S--; 
              } 
            }
            else { 
              tmpres16S = (quadlow2 & 0xFFFF); 
            } 
          }
          else if (tmp <= -16) { 
            tmpres16S = (int16_t)(quadlow2 & 0xFFFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres16S = (int16_t)(quadlow2 & 0xFFFF) >> -tmp; 
          }
          else { 
            tmpres16S = (quadlow2 & 0xFFFF) << tmp; 
            if (((int16_t)tmpres16S >> tmp) != (int16_t)(quadlow2 & 0xFFFF)) { 
              sat = 1;
              tmpres16S = (uint32_t)(1 << (16 - 1)); 
              if ((int16_t)(quadlow2 & 0xFFFF) > 0) { 
                tmpres16S--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadlow>>16) & 0xFFFF); 
          if (tmp >= 16) { 
            if ((quadlow2>>16) & 0xFFFF) { 
              sat = 1;
              qlresult = (uint32_t)(1 << (16 - 1)); 
              if (((quadlow2>>16) & 0xFFFF) > 0) { 
                qlresult--; 
              } 
            }
            else { 
              qlresult = ((quadlow2>>16) & 0xFFFF); 
            } 
          }
          else if (tmp <= -16) { 
            qlresult = (int16_t)((quadlow2>>16) & 0xFFFF) >> 15; 
          } 
          else if (tmp < 0) { 
            qlresult = (int16_t)((quadlow2>>16) & 0xFFFF) >> -tmp; 
          }
          else { 
            qlresult = ((quadlow2>>16) & 0xFFFF) << tmp; 
            if (((int16_t)lresult >> tmp) != (int16_t)((quadlow2>>16) & 0xFFFF)) { 
              sat = 1;
              qlresult = (uint32_t)(1 << (16 - 1)); 
              if ((int16_t)((quadlow2>>16) & 0xFFFF) > 0) { 
                qlresult--; 
              } 
            } 
          }

          qlresult = qlresult & 0xFFFF;
          qlresult = ((uint32_t)qlresult << 16) | tmpres16S;

          tmp = (immS) ? shift_amount : (int8_t)(quadhigh & 0xFFFF); 
          if (tmp >= 16) { 
            if (quadhigh2 & 0xFFFF) { 
              sat = 1;
              tmpres16S = (uint32_t)(1 << (16 - 1)); 
              if ((quadhigh2 & 0xFFFF) > 0) { 
                tmpres16S--; 
              } 
            }
            else { 
              tmpres16S = (quadhigh2 & 0xFFFF); 
            } 
          }
          else if (tmp <= -16) { 
            tmpres16S = (int16_t)(quadhigh2 & 0xFFFF) >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres16S = (int16_t)(quadhigh2 & 0xFFFF) >> -tmp; 
          }
          else { 
            tmpres16S = (quadhigh2 & 0xFFFF) << tmp; 
            if (((int16_t)tmpres16S >> tmp) != (int16_t)(quadhigh2 & 0xFFFF)) { 
              sat = 1;
              tmpres16S = (uint32_t)(1 << (16 - 1)); 
              if ((int16_t)(quadhigh2 & 0xFFFF) > 0) { 
                tmpres16S--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)((quadhigh>>16) & 0xFFFF); 
          if (tmp >= 16) { 
            if ((quadhigh2>>16) & 0xFFFF) { 
              sat = 1;
              qhresult = (uint32_t)(1 << (16 - 1)); 
              if (((quadhigh2>>16) & 0xFFFF) > 0) { 
                qhresult--; 
              } 
            }
            else { 
              qhresult = ((quadhigh2>>16) & 0xFFFF); 
            } 
          }
          else if (tmp <= -16) { 
            qhresult = (int16_t)((quadhigh2>>16) & 0xFFFF) >> 15; 
          } 
          else if (tmp < 0) { 
            qhresult = (int16_t)((quadhigh2>>16) & 0xFFFF) >> -tmp; 
          }
          else { 
            qhresult = ((quadhigh2>>16) & 0xFFFF) << tmp; 
            if (((int16_t)qhresult >> tmp) != (int16_t)((quadhigh2>>16) & 0xFFFF)) { 
              sat = 1;
              qhresult = (uint32_t)(1 << (16 - 1)); 
              if ((int16_t)((quadhigh2>>16) & 0xFFFF) > 0) { 
                qhresult--; 
              } 
            } 
          }

          qhresult = qhresult & 0xFFFF;
          qhresult = ((uint32_t)qhresult << 16) | tmpres16S;
        }
      } //end signed
      break;
    case 2: //32-bit
      if(is_unsigned) {
        tmp = (immS) ? shift_amount : (int8_t)low;

        if(qshlu) {
          if ((int32_t)low2 < 0) {
            sat = 1;
            lresult = 0;
            done = true;
          }
        }

        if(!done) {
          if (tmp >= 32) { 
            if (low2) { 
              sat = 1;
              lresult = ~0; 
            }
            else { 
              lresult = 0; 
            } 
          }
          else if (tmp <= -32) { 
            lresult = 0; 
          }
          else if (tmp < 0) { 
            lresult = low2 >> -tmp; 
          }
          else { 
            lresult = low2 << tmp; 
            if ((lresult >> tmp) != low2) { 
              sat = 1;
              lresult = ~0; 
            } 
          }
        }

        done = false;
        tmp = (immS) ? shift_amount : (int8_t)high;

        if(qshlu) {
          if ((int32_t)high2 < 0) {
            sat = 1;
            hresult = 0;
            done = true;
          }
        }

        if(!done) {
          if (tmp >= 32) { 
            if (high2) { 
              sat = 1;
              hresult = ~0; 
            }
            else { 
              hresult = 0; 
            } 
          }
          else if (tmp <= -32) { 
            hresult = 0; 
          }
          else if (tmp < 0) { 
            hresult = high2 >> -tmp; 
          }
          else { 
            hresult = high2 << tmp; 
            if ((hresult >> tmp) != high2) { 
              sat = 1;
              hresult = ~0; 
            } 
          }
        }

        if(quad) {
          done = false;
          tmp = (immS) ? shift_amount : (int8_t)quadlow;

          if(qshlu) {
            if ((int32_t)quadlow2 < 0) {
              sat = 1;
              qlresult = 0;
              done = true;
            }
          }

          if(!done) {
            if (tmp >= 32) { 
              if (quadlow2) { 
                sat = 1;
                qlresult = ~0; 
              }
              else { 
                qlresult = 0; 
              } 
            }
            else if (tmp <= -32) { 
              qlresult = 0; 
            }
            else if (tmp < 0) { 
              qlresult = quadlow2 >> -tmp; 
            }
            else { 
              qlresult = quadlow2 << tmp; 
              if ((qlresult >> tmp) != quadlow2) { 
                sat = 1;
                qlresult = ~0; 
              } 
            }
          }

          done = false;
          tmp = (immS) ? shift_amount : (int8_t)quadhigh;

          if(qshlu) {
            if ((int32_t)quadhigh2 < 0) {
              sat = 1;
              qhresult = 0;
              done = true;
            }
          }

          if(!done) {
            if (tmp >= 32) { 
              if (quadhigh2) { 
                sat = 1;
                qhresult = ~0; 
              }
              else { 
                qhresult = 0; 
              } 
            }
            else if (tmp <= -32) { 
              qhresult = 0; 
            }
            else if (tmp < 0) { 
              qhresult = quadhigh2 >> -tmp; 
            }
            else { 
              qhresult = quadhigh2 << tmp; 
              if ((qhresult >> tmp) != quadhigh2) { 
                sat = 1;
                qhresult = ~0; 
              } 
            }
          }
        }
      } //end unsigned
      else { //signed
        tmp = (immS) ? shift_amount : (int8_t)low; 
        if (tmp >= 32) { 
          if (low2) { 
            sat = 1;
            lresult = (uint32_t)(1 << (32 - 1)); 
            if (low2 > 0) { 
              lresult--; 
            } 
          }
          else { 
            lresult = low2; 
          } 
        }
        else if (tmp <= -32) { 
          lresult = (int32_t)low2 >> 31; 
        } 
        else if (tmp < 0) { 
          lresult = (int32_t)low2 >> -tmp; 
        }
        else { 
          lresult = low2 << tmp; 
          if (((int32_t)lresult >> tmp) != (int32_t)low2) { 
            sat = 1;
            lresult = (uint32_t)(1 << (32 - 1)); 
            if ((int32_t)low2 > 0) { 
              lresult--; 
            } 
          } 
        }

        tmp = (immS) ? shift_amount : (int8_t)high; 
        if (tmp >= 32) { 
          if (high2) { 
            sat = 1;
            hresult = (uint32_t)(1 << (32 - 1)); 
            if ((int32_t)high2 > 0) { 
              hresult--; 
            } 
          }
          else { 
            hresult = high2; 
          } 
        }
        else if (tmp <= -32) { 
          hresult = (int32_t)high2 >> 31; 
        } 
        else if (tmp < 0) { 
          hresult = (int32_t)high2 >> -tmp; 
        }
        else { 
          hresult = high2 << tmp; 
          if (((int32_t)hresult >> tmp) != (int32_t)high2) { 
            sat = 1;
            hresult = (uint32_t)(1 << (32 - 1)); 
            if ((int32_t)high2 > 0) { 
              hresult--; 
            } 
          } 
        }

        if(quad) {
          tmp = (immS) ? shift_amount : (int8_t)quadlow; 
          if (tmp >= 32) { 
            if (quadlow2) { 
              sat = 1;
              qlresult = (uint32_t)(1 << (32 - 1)); 
              if ((int32_t)quadlow2 > 0) { 
                qlresult--; 
              } 
            }
            else { 
              qlresult = quadlow2; 
            } 
          }
          else if (tmp <= -32) { 
            qlresult = (int32_t)quadlow2 >> 31; 
          } 
          else if (tmp < 0) { 
            qlresult = (int32_t)quadlow2 >> -tmp; 
          }
          else { 
            lresult = quadlow2 << tmp; 
            if (((int32_t)lresult >> (int32_t)tmp) != (int32_t)quadlow2) { 
              sat = 1;
              qlresult = (uint32_t)(1 << (32 - 1)); 
              if ((int32_t)quadlow2 > 0) { 
                qlresult--; 
              } 
            } 
          }

          tmp = (immS) ? shift_amount : (int8_t)quadhigh; 
          if (tmp >= 32) { 
            if (quadhigh2) { 
              sat = 1;
              qhresult = (uint32_t)(1 << (32 - 1)); 
              if ((int32_t)quadhigh2 > 0) { 
                qhresult--; 
              } 
            }
            else { 
              qhresult = quadhigh2; 
            } 
          }
          else if (tmp <= -32) { 
            qhresult = (int32_t)quadhigh2 >> 31; 
          } 
          else if (tmp < 0) { 
            qhresult = (int32_t)quadhigh2 >> -tmp; 
          }
          else { 
            qhresult = quadhigh2 << tmp; 
            if (((int32_t)qhresult >> tmp) != (int32_t)quadhigh2) { 
              sat = 1;
              qhresult = (uint32_t)(1 << (32 - 1)); 
              if ((int32_t)quadhigh2 > 0) { 
                qhresult--; 
              } 
            } 
          }
        }
      } //end signed
      break;
    case 3: //64-bit
      if(is_unsigned) {
        shift = (immS) ? shift_amount : (int8_t)low;
        valU = ((uint64_t)high2 << 32) | low2;

        if(qshlu) {
          if ((int64_t)valU < 0) {
              sat = 1;
              valU = 0;
              done = true;
          }
        }

        if(!done) {
          if (shift >= 64) {
            if (valU) {
              valU = ~(uint64_t)0;
              sat = 1;
            }
          } 
          else if (shift <= -64) {
            valU = 0;
          } 
          else if (shift < 0) {
            valU >>= -shift;
          } 
          else {
            uint64_t tmp = valU;
            valU <<= shift;
            if ((valU >> shift) != tmp) {
              sat = 1;
              valU = ~(uint64_t)0;
            }
          }
        }

        lresult = valU & 0xFFFFFFFF;
        hresult = (valU >> 32) & 0xFFFFFFFFF;

        if(quad) {
          done = false;
          shift = (immS) ? shift_amount : (int8_t)quadlow;
          valU = ((uint64_t)quadhigh2 << 32) | quadlow2;

          if(qshlu) {
            if ((int64_t)valU < 0) {
                sat = 1;
                valU = 0;
                done = true;
            }
          }

          if(!done) {
            if (shift >= 64) {
              if (valU) {
                valU = ~(uint64_t)0;
                sat = 1;
              }
            } 
            else if (shift <= -64) {
              valU = 0;
            } 
            else if (shift < 0) {
              valU >>= -shift;
            } 
            else {
              uint64_t tmp = valU;
              valU <<= shift;
              if ((valU >> shift) != tmp) {
                sat = 1;
                valU = ~(uint64_t)0;
              }
            }
          }

          qlresult = valU & 0xFFFFFFFF;
          qhresult = (valU >> 32) & 0xFFFFFFFFF;
        }
      } //end unsigned
      else { //signed
        shift = (immS) ? shift_amount : (uint8_t)low;
        valS = ((int64_t)high2 << 32) | low2;

        if (shift >= 64) {
          if (valS) {
            sat = 1;
            valS = (valS >> 63) ^ ~SIGNBIT64;
          }
        }
        else if (shift <= -64) {
          valS >>= 63;
        }
        else if (shift < 0) {
          valS >>= -shift;
        }
        else {
          int64_t tmp = valS;
          valS <<= shift;
          if ((valS >> shift) != tmp) {
            sat = 1;
            valS = (tmp >> 63) ^ ~SIGNBIT64;
          }
        }

        lresult = valS & 0xFFFFFFFF;
        hresult = (valS >> 32) & 0xFFFFFFFFF;

        if(quad) {
          shift = (immS) ? shift_amount : (uint8_t)quadlow;
          valS = ((int64_t)quadhigh2 << 32) | quadlow2;

          if (shift >= 64) {
            if (valS) {
              sat = 1;
              valS = (valS >> 63) ^ ~SIGNBIT64;
            }
          }
          else if (shift <= -64) {
            valS >>= 63;
          }
          else if (shift < 0) {
            valS >>= -shift;
          }
          else {
            int64_t tmp = valS;
            valS <<= shift;
            if ((valS >> shift) != tmp) {
              sat = 1;
              valS = (tmp >> 63) ^ ~SIGNBIT64;
            }
          }

          qlresult = valS & 0xFFFFFFFF;
          qhresult = (valS >> 32) & 0xFFFFFFFFF;
        }
      } //end signed
      break;
  }

  prog->st32( arg1 + 16, lresult );
  prog->st32( arg1 + 20, hresult );

  if(quad) {
    prog->st32( arg1 + 32, sat);
    prog->st32( arg1 + 24, qlresult );
    prog->st32( arg1 + 28, qhresult );
  }
  else {
    prog->st32( arg1 + 24, sat);
  }

  return 0;
}

uint32_t do_vqrshl(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low      = prog->ldu32( arg1+4 ) ;
  uint32_t high     = prog->ldu32( arg1+8 ) ;
  uint32_t imm      = prog->ldu32( arg1+12 ) ;

  uint32_t low2;
  uint32_t high2;
  uint32_t quadlow;
  uint32_t quadhigh;
  uint32_t quadlow2;
  uint32_t quadhigh2;

  uint8_t size = (imm & 0x3);
  uint8_t quad = (imm >> 2) & 0x1;
  uint8_t is_unsigned = (imm>>3) & 0x1;

  uint8_t tmpres8U;
  int8_t tmpres8S, tmpres8S2, tmpres8S3, tmpres8S4;

  uint16_t tmpres16U;
  int16_t tmpres16S, tmpres16S2;

  uint32_t lresult;
  uint32_t hresult;
  uint32_t qlresult;
  uint32_t qhresult;

  uint64_t valU;
  int64_t valS;

  uint64_t tmpU;
  uint32_t tmp32U;

  int64_t tmpS;
  int8_t tmp8S;
  uint8_t src8;
  uint16_t src16;

  int8_t shift;
  uint8_t sat = 0;

  if(quad) {
    low2        = prog->ldu32( arg1+44 ) ;
    high2       = prog->ldu32( arg1+48 ) ;

    quadlow    = prog->ldu32( arg1+36 ) ;
    quadhigh   = prog->ldu32( arg1+40 ) ;

    quadlow2    = prog->ldu32( arg1+52 ) ;
    quadhigh2   = prog->ldu32( arg1+56 ) ;
  }
  else {
    low2  = prog->ldu32( arg1+28 ) ;
    high2 = prog->ldu32( arg1+32 ) ;
  }

  switch(size) {
    case 0: //8-bit
      if(is_unsigned) { //unsigned
        //FIXME: this is wrong in QEMU
        src8 = low2 & 0xFF;
        tmp8S = (int8_t)(low & 0xFF); 
        if (tmp8S >= 8) { 
          if (src8) { 
            sat = 1;
            lresult = 0xFF; 
          } 
          else { 
            lresult = 0; 
          } 
        } 
        else if (tmp8S < -8) { 
          lresult = 0; 
        } 
        else if (tmp8S == -8) { 
          lresult = src8 >> 7; 
        } 
        else if (tmp8S < 0) { 
          lresult = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          lresult = src8 << tmp8S; 
          if ((lresult >> tmp8S) != src8) { 
            sat = 1;
            lresult = 0xFF; 
          } 
        }

        src8 = (low2>>8) & 0xFF;
        tmp8S = (int8_t)((low>>8) & 0xFF); 
        if (tmp8S >= 8) { 
          if (src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
          else { 
            tmpres8U = 0; 
          } 
        } 
        else if (tmp8S < -8) { 
          tmpres8U = 0; 
        } 
        else if (tmp8S == -8) { 
          tmpres8U = src8 >> 7; 
        } 
        else if (tmp8S < 0) { 
          tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          tmpres8U = src8 << tmp8S; 
          if ((tmpres8U >> tmp8S) != src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
        }

        lresult |= (((uint32_t)tmpres8U) << 8);

        src8 = (low2>>16) & 0xFF;
        tmp8S = (int8_t)((low>>16) & 0xFF); 
        if (tmp8S >= 8) { 
          if (src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
          else { 
            tmpres8U = 0; 
          } 
        } 
        else if (tmp8S < -8) { 
          tmpres8U = 0; 
        } 
        else if (tmp8S == -8) { 
          tmpres8U = src8 >> 7; 
        } 
        else if (tmp8S < 0) { 
          tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          tmpres8U = src8 << tmp8S; 
          if ((tmpres8U >> tmp8S) != src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
        }

        lresult |= (((uint32_t)tmpres8U) << 16);

        src8 = (low2>>24) & 0xFF;
        tmp8S = (int8_t)((low>>24) & 0xFF); 
        if (tmp8S >= 8) { 
          if (src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
          else { 
            tmpres8U = 0; 
          } 
        } 
        else if (tmp8S < -8) { 
          tmpres8U = 0; 
        } 
        else if (tmp8S == -8) { 
          tmpres8U = src8 >> 7; 
        } 
        else if (tmp8S < 0) { 
          tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          tmpres8U = src8 << tmp8S; 
          if ((tmpres8U >> tmp8S) != src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
        }

        lresult |= (((uint32_t)tmpres8U) << 24);

        src8 = high2 & 0xFF;
        tmp8S = (int8_t)(high & 0xFF); 
        if (tmp8S >= 8) { 
          if (src8) { 
            sat = 1;
            hresult = 0xFF; 
          } 
          else { 
            hresult = 0; 
          } 
        } 
        else if (tmp8S < -8) { 
          hresult = 0; 
        } 
        else if (tmp8S == -8) { 
          hresult = src8 >> 7; 
        } 
        else if (tmp8S < 0) { 
          hresult = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          hresult = src8 << tmp8S; 
          if ((hresult >> tmp8S) != src8) { 
            sat = 1;
            hresult = 0xFF; 
          } 
        }

        src8 = (high2>>8) & 0xFF;
        tmp8S = (int8_t)((high>>8) & 0xFF); 
        if (tmp8S >= 8) { 
          if (src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
          else { 
            tmpres8U = 0; 
          } 
        } 
        else if (tmp8S < -8) { 
          tmpres8U = 0; 
        } 
        else if (tmp8S == -8) { 
          tmpres8U = src8 >> 7; 
        } 
        else if (tmp8S < 0) { 
          tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          tmpres8U = src8 << tmp8S; 
          if ((tmpres8U >> tmp8S) != src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
        }

        hresult |= (((uint32_t)tmpres8U) << 8);

        src8 = (high2>>16) & 0xFF;
        tmp8S = (int8_t)((high>>16) & 0xFF); 
        if (tmp8S >= 8) { 
          if (src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
          else { 
            tmpres8U = 0; 
          } 
        } 
        else if (tmp8S < -8) { 
          tmpres8U = 0; 
        } 
        else if (tmp8S == -8) { 
          tmpres8U = src8 >> 7; 
        } 
        else if (tmp8S < 0) { 
          tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          tmpres8U = src8 << tmp8S; 
          if ((tmpres8U >> tmp8S) != src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
        }

        hresult |= (((uint32_t)tmpres8U) << 16);

        src8 = (high2>>24) & 0xFF;
        tmp8S = (int8_t)((high>>24) & 0xFF); 
        if (tmp8S >= 8) { 
          if (src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
          else { 
            tmpres8U = 0; 
          } 
        } 
        else if (tmp8S < -8) { 
          tmpres8U = 0; 
        } 
        else if (tmp8S == -8) { 
          tmpres8U = src8 >> 7; 
        } 
        else if (tmp8S < 0) { 
          tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          tmpres8U = src8 << tmp8S; 
          if ((tmpres8U >> tmp8S) != src8) { 
            sat = 1;
            tmpres8U = 0xFF; 
          } 
        }

        hresult |= (((uint32_t)tmpres8U) << 24);

        if(quad) {
          src8 = quadlow2 & 0xFF;
          tmp8S = (int8_t)(quadlow & 0xFF); 
          if (tmp8S >= 8) { 
            if (src8) { 
              sat = 1;
              qlresult = 0xFF; 
            } 
            else { 
              qlresult = 0; 
            } 
          } 
          else if (tmp8S < -8) { 
            qlresult = 0; 
          } 
          else if (tmp8S == -8) { 
            qlresult = src8 >> 7; 
          } 
          else if (tmp8S < 0) { 
            qlresult = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            qlresult = src8 << tmp8S; 
            if ((qlresult >> tmp8S) != src8) { 
              sat = 1;
              qlresult = 0xFF; 
            } 
          }

          src8 = (quadlow2>>8) & 0xFF;
          tmp8S = (int8_t)((quadlow>>8) & 0xFF); 
          if (tmp8S >= 8) { 
            if (src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
            else { 
              tmpres8U = 0; 
            } 
          } 
          else if (tmp8S < -8) { 
            tmpres8U = 0; 
          } 
          else if (tmp8S == -8) { 
            tmpres8U = src8 >> 7; 
          } 
          else if (tmp8S < 0) { 
            tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            tmpres8U = src8 << tmp8S; 
            if ((tmpres8U >> tmp8S) != src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
          }

          qlresult |= (((uint32_t)tmpres8U) << 8);

          src8 = (quadlow2>>16) & 0xFF;
          tmp8S = (int8_t)((quadlow>>16) & 0xFF); 
          if (tmp8S >= 8) { 
            if (src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
            else { 
              tmpres8U = 0; 
            } 
          } 
          else if (tmp8S < -8) { 
            tmpres8U = 0; 
          } 
          else if (tmp8S == -8) { 
            tmpres8U = src8 >> 7; 
          } 
          else if (tmp8S < 0) { 
            tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            tmpres8U = src8 << tmp8S; 
            if ((tmpres8U >> tmp8S) != src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
          }

          qlresult |= (((uint32_t)tmpres8U) << 16);

          src8 = (quadlow2>>24) & 0xFF;
          tmp8S = (int8_t)((quadlow>>24) & 0xFF); 
          if (tmp8S >= 8) { 
            if (src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
            else { 
              tmpres8U = 0; 
            } 
          } 
          else if (tmp8S < -8) { 
            tmpres8U = 0; 
          } 
          else if (tmp8S == -8) { 
            tmpres8U = src8 >> 7; 
          } 
          else if (tmp8S < 0) { 
            tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            tmpres8U = src8 << tmp8S; 
            if ((tmpres8U >> tmp8S) != src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
          }

          qlresult |= (((uint32_t)tmpres8U) << 24);

          src8 = quadhigh2 & 0xFF;
          tmp8S = (int8_t)(quadhigh & 0xFF); 
          if (tmp8S >= 8) { 
            if (src8) { 
              sat = 1;
              qhresult = 0xFF; 
            } 
            else { 
              qhresult = 0; 
            } 
          } 
          else if (tmp8S < -8) { 
            qhresult = 0; 
          } 
          else if (tmp8S == -8) { 
            qhresult = src8 >> 7; 
          } 
          else if (tmp8S < 0) { 
            qhresult = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            hresult = src8 << tmp8S; 
            if ((hresult >> tmp8S) != src8) { 
              sat = 1;
              qhresult = 0xFF; 
            } 
          }

          src8 = (quadhigh2>>8) & 0xFF;
          tmp8S = (int8_t)((quadhigh>>8) & 0xFF); 
          if (tmp8S >= 8) { 
            if (src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
            else { 
              tmpres8U = 0; 
            } 
          } 
          else if (tmp8S < -8) { 
            tmpres8U = 0; 
          } 
          else if (tmp8S == -8) { 
            tmpres8U = src8 >> 7; 
          } 
          else if (tmp8S < 0) { 
            tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            tmpres8U = src8 << tmp8S; 
            if ((tmpres8U >> tmp8S) != src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
          }

          qhresult |= (((uint32_t)tmpres8U) << 8);

          src8 = (quadhigh2>>16) & 0xFF;
          tmp8S = (int8_t)((quadhigh>>16) & 0xFF); 
          if (tmp8S >= 8) { 
            if (src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
            else { 
              tmpres8U = 0; 
            } 
          } 
          else if (tmp8S < -8) { 
            tmpres8U = 0; 
          } 
          else if (tmp8S == -8) { 
            tmpres8U = src8 >> 7; 
          } 
          else if (tmp8S < 0) { 
            tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            tmpres8U = src8 << tmp8S; 
            if ((tmpres8U >> tmp8S) != src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
          }

          qhresult |= (((uint32_t)tmpres8U) << 16);

          src8 = (quadhigh2>>24) & 0xFF;
          tmp8S = (int8_t)((quadhigh>>24) & 0xFF); 
          if (tmp8S >= 8) { 
            if (src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
            else { 
              tmpres8U = 0; 
            } 
          } 
          else if (tmp8S < -8) { 
            tmpres8U = 0; 
          } 
          else if (tmp8S == -8) { 
            tmpres8U = src8 >> 7; 
          } 
          else if (tmp8S < 0) { 
            tmpres8U = (src8 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            tmpres8U = src8 << tmp8S; 
            if ((tmpres8U >> tmp8S) != src8) { 
              sat = 1;
              tmpres8U = 0xFF; 
            } 
          }

          qhresult |= (((uint32_t)tmpres8U) << 24);
        }
      } //end unsigned
      else { //signed
        tmp8S = (int8_t)(low & 0xFF); 
        if (tmp8S < 0) { 
          tmpres8S = ((low2 & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres8S = (low2 & 0xFF) << tmp8S; 
          if ((tmpres8S >> tmp8S) != (int8_t)(low2 & 0xFF)) { 
            sat = 1;
            tmpres8S = (uint32_t)(1 << (8 - 1)) - ((low2 & 0xFF) > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)((low>>8) & 0xFF); 
        if (tmp8S < 0) { 
          tmpres8S2 = (((low2>>8) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres8S2 = ((low2>>8) & 0xFF) << tmp8S; 
          if ((tmpres8S2 >> tmp8S) != (int8_t)((low2>>8) & 0xFF)) { 
            sat = 1;
            tmpres8S2 = (uint32_t)(1 << (8 - 1)) - (((low2>>8) & 0xFF) > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)((low>>16) & 0xFF); 
        if (tmp8S < 0) { 
          tmpres8S3 = (((low2>>16) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres8S3 = ((low2>>16) & 0xFF) << tmp8S; 
          if ((tmpres8S3 >> tmp8S) != (int8_t)((low2>>16) & 0xFF)) { 
            sat = 1;
            tmpres8S3 = (uint32_t)(1 << (8 - 1)) - (((low2>>16) & 0xFF) > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)((low>>24) & 0xFF); 
        if (tmp8S < 0) { 
          tmpres8S4 = (((low2>>24) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres8S4 = ((low2>>24) & 0xFF) << tmp8S; 
          if ((tmpres8S4 >> tmp8S) != (int8_t)((low2>>24) & 0xFF)) { 
            sat = 1;
            tmpres8S4 = (uint32_t)(1 << (8 - 1)) - (((low2>>24) & 0xFF) > 0 ? 1 : 0); 
          } 
        }

        lresult = ((uint32_t)tmpres8S4 << 24) | ((uint32_t)tmpres8S3 << 16) | ((uint32_t)tmpres8S2 << 8) | tmpres8S;

        tmp8S = (int8_t)(high & 0xFF); 
        if (tmp8S < 0) { 
          tmpres8S = ((high2 & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres8S = (high2 & 0xFF) << tmp8S; 
          if ((tmpres8S >> tmp8S) != (int8_t)(high2 & 0xFF)) { 
            sat = 1;
            tmpres8S = (uint32_t)(1 << (8 - 1)) - ((high2 & 0xFF) > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)((high>>8) & 0xFF); 
        if (tmp8S < 0) { 
          tmpres8S2 = (((high2>>8) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres8S2 = ((high2>>8) & 0xFF) << tmp8S; 
          if ((tmpres8S2 >> tmp8S) != (int8_t)((high2>>8) & 0xFF)) { 
            sat = 1;
            tmpres8S2 = (uint32_t)(1 << (8 - 1)) - (((high2>>8) & 0xFF) > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)((high>>16) & 0xFF); 
        if (tmp8S < 0) { 
          tmpres8S3 = (((high2>>16) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres8S3 = ((high2>>16) & 0xFF) << tmp8S; 
          if ((tmpres8S3 >> tmp8S) != (int8_t)((high2>>16) & 0xFF)) { 
            sat = 1;
            tmpres8S3 = (uint32_t)(1 << (8 - 1)) - (((high2>>16) & 0xFF) > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)((high>>24) & 0xFF); 
        if (tmp8S < 0) { 
          tmpres8S4 = (((high2>>24) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres8S4 = ((high2>>24) & 0xFF) << tmp8S; 
          if ((tmpres8S4 >> tmp8S) != (int8_t)((high2>>24) & 0xFF)) { 
            sat = 1;
            tmpres8S4 = (uint32_t)(1 << (8 - 1)) - (((high2>>24) & 0xFF) > 0 ? 1 : 0); 
          } 
        }

        hresult = ((uint32_t)tmpres8S4 << 24) | ((uint32_t)tmpres8S3 << 16) | ((uint32_t)tmpres8S2 << 8) | tmpres8S;

        if(quad) {
          tmp8S = (int8_t)(quadlow & 0xFF); 
          if (tmp8S < 0) { 
            tmpres8S = ((quadlow2 & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres8S = (quadlow2 & 0xFF) << tmp8S; 
            if ((tmpres8S >> tmp8S) != (int8_t)(quadlow2 & 0xFF)) { 
              sat = 1;
              tmpres8S = (uint32_t)(1 << (8 - 1)) - ((quadlow2 & 0xFF) > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)((quadlow>>8) & 0xFF); 
          if (tmp8S < 0) { 
            tmpres8S2 = (((quadlow2>>8) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres8S2 = ((quadlow2>>8) & 0xFF) << tmp8S; 
            if ((tmpres8S2 >> tmp8S) != (int8_t)((quadlow2>>8) & 0xFF)) { 
              sat = 1;
              tmpres8S2 = (uint32_t)(1 << (8 - 1)) - (((quadlow2>>8) & 0xFF) > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)((quadlow>>16) & 0xFF); 
          if (tmp8S < 0) { 
            tmpres8S3 = (((quadlow2>>16) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres8S3 = ((quadlow2>>16) & 0xFF) << tmp8S; 
            if ((tmpres8S3 >> tmp8S) != (int8_t)((quadlow2>>16) & 0xFF)) { 
              sat = 1;
              tmpres8S3 = (uint32_t)(1 << (8 - 1)) - (((quadlow2>>16) & 0xFF) > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)((quadlow>>24) & 0xFF); 
          if (tmp8S < 0) { 
            tmpres8S4 = (((quadlow2>>24) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres8S4 = ((quadlow2>>24) & 0xFF) << tmp8S; 
            if ((tmpres8S4 >> tmp8S) != (int8_t)((quadlow2>>24) & 0xFF)) { 
              sat = 1;
              tmpres8S4 = (uint32_t)(1 << (8 - 1)) - (((quadlow2>>24) & 0xFF) > 0 ? 1 : 0); 
            } 
          }

          qlresult = ((uint32_t)tmpres8S4 << 24) | ((uint32_t)tmpres8S3 << 16) | ((uint32_t)tmpres8S2 << 8) | tmpres8S;

          tmp8S = (int8_t)(quadhigh & 0xFF); 
          if (tmp8S < 0) { 
            tmpres8S = ((quadhigh2 & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres8S = (quadhigh2 & 0xFF) << tmp8S; 
            if ((tmpres8S >> tmp8S) != (int8_t)(quadhigh2 & 0xFF)) { 
              sat = 1;
              tmpres8S = (uint32_t)(1 << (8 - 1)) - ((quadhigh2 & 0xFF) > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)((quadhigh>>8) & 0xFF); 
          if (tmp8S < 0) { 
            tmpres8S2 = (((quadhigh2>>8) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres8S2 = ((quadhigh2>>8) & 0xFF) << tmp8S; 
            if ((tmpres8S2 >> tmp8S) != (int8_t)((quadhigh2>>8) & 0xFF)) { 
              sat = 1;
              tmpres8S2 = (uint32_t)(1 << (8 - 1)) - (((quadhigh2>>8) & 0xFF) > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)((quadhigh>>16) & 0xFF); 
          if (tmp8S < 0) { 
            tmpres8S3 = (((quadhigh2>>16) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres8S3 = ((quadhigh2>>16) & 0xFF) << tmp8S; 
            if ((tmpres8S3 >> tmp8S) != (int8_t)((quadhigh2>>16) & 0xFF)) { 
              sat = 1;
              tmpres8S3 = (uint32_t)(1 << (8 - 1)) - (((quadhigh2>>16) & 0xFF) > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)((quadhigh>>24) & 0xFF); 
          if (tmp8S < 0) { 
            tmpres8S4 = (((quadhigh2>>24) & 0xFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres8S4 = ((quadhigh2>>24) & 0xFF) << tmp8S; 
            if ((tmpres8S4 >> tmp8S) != (int8_t)((quadhigh2>>24) & 0xFF)) { 
              sat = 1;
              tmpres8S4 = (uint32_t)(1 << (8 - 1)) - (((quadhigh2>>24) & 0xFF) > 0 ? 1 : 0); 
            } 
          }

          qhresult = ((uint32_t)tmpres8S4 << 24) | ((uint32_t)tmpres8S3 << 16) | ((uint32_t)tmpres8S2 << 8) | tmpres8S;
        }
      } //end signed
      break;
    case 1: //16-bit
      if(is_unsigned) { //unsigned
        //FIXME: this is wrong in QEMU
        src16 = low2 & 0xFFFF;
        tmp8S = (int8_t)(low & 0xFFFF); 
        if (tmp8S >= 16) { 
          if (src16) { 
            sat = 1;
            lresult = 0xFFFF; 
          } 
          else { 
            lresult = 0; 
          } 
        } 
        else if (tmp8S < -16) { 
          lresult = 0; 
        } 
        else if (tmp8S == -16) { 
          lresult = src16 >> 15; 
        } 
        else if (tmp8S < 0) { 
          lresult = (src16 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          lresult = src16 << tmp8S; 
          if ((lresult >> tmp8S) != src16) { 
            sat = 1;
            lresult = 0xFFFF;
          } 
        }

        src16 = (low2>>16) & 0xFFFF;
        tmp8S = (int8_t)((low>>16) & 0xFFFF); 
        if (tmp8S >= 16) { 
          if (src16) { 
            sat = 1;
            tmpres16U = 0xFFFF; 
          } 
          else { 
            tmpres16U = 0; 
          } 
        } 
        else if (tmp8S < -16) { 
          tmpres16U = 0; 
        } 
        else if (tmp8S == -16) { 
          tmpres16U = src16 >> 15; 
        } 
        else if (tmp8S < 0) { 
          tmpres16U = (src16 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          tmpres16U = src16 << tmp8S; 
          if ((tmpres16U >> tmp8S) != src16) { 
            sat = 1;
            tmpres16U = 0xFFFF;
          } 
        }

        lresult |= ((uint32_t)tmpres16U << 16);

        src16 = high2 & 0xFFFF;
        tmp8S = (int8_t)(high & 0xFFFF); 
        if (tmp8S >= 16) { 
          if (src16) { 
            sat = 1;
            hresult = 0xFFFF; 
          } 
          else { 
            hresult = 0; 
          } 
        } 
        else if (tmp8S < -16) { 
          hresult = 0; 
        } 
        else if (tmp8S == -16) { 
          hresult = src16 >> 15; 
        } 
        else if (tmp8S < 0) { 
          hresult = (src16 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          hresult = src16 << tmp8S; 
          if ((hresult >> tmp8S) != src16) { 
            sat = 1;
            hresult = 0xFFFF;
          } 
        }

        src16 = (high2>>16) & 0xFFFF;
        tmp8S = (int8_t)((high>>16) & 0xFFFF); 
        if (tmp8S >= 16) { 
          if (src16) { 
            sat = 1;
            tmpres16U = 0xFFFF; 
          } 
          else { 
            tmpres16U = 0; 
          } 
        } 
        else if (tmp8S < -16) { 
          tmpres16U = 0; 
        } 
        else if (tmp8S == -16) { 
          tmpres16U = src16 >> 15; 
        } 
        else if (tmp8S < 0) { 
          tmpres16U = (src16 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        } 
        else { 
          tmpres16U = src16 << tmp8S; 
          if ((tmpres16U >> tmp8S) != src16) { 
            sat = 1;
            tmpres16U = 0xFFFF;
          } 
        }

        hresult |= ((uint32_t)tmpres16U << 16);

        if(quad) {
          src16 = quadlow2 & 0xFFFF;
          tmp8S = (int8_t)(quadlow & 0xFFFF); 
          if (tmp8S >= 16) { 
            if (src16) { 
              sat = 1;
              qlresult = 0xFFFF; 
            } 
            else { 
              qlresult = 0; 
            } 
          } 
          else if (tmp8S < -16) { 
            qlresult = 0; 
          } 
          else if (tmp8S == -16) { 
            qlresult = src16 >> 15; 
          } 
          else if (tmp8S < 0) { 
            qlresult = (src16 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            qlresult = src16 << tmp8S; 
            if ((qlresult >> tmp8S) != src16) { 
              sat = 1;
              qlresult = 0xFFFF;
            } 
          }

          src16 = (quadlow2>>16) & 0xFFFF;
          tmp8S = (int8_t)((quadlow>>16) & 0xFFFF); 
          if (tmp8S >= 16) { 
            if (src16) { 
              sat = 1;
              tmpres16U = 0xFFFF; 
            } 
            else { 
              tmpres16U = 0; 
            } 
          } 
          else if (tmp8S < -16) { 
            tmpres16U = 0; 
          } 
          else if (tmp8S == -16) { 
            tmpres16U = src16 >> 15; 
          } 
          else if (tmp8S < 0) { 
            tmpres16U = (src16 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            tmpres16U = src16 << tmp8S; 
            if ((tmpres16U >> tmp8S) != src16) { 
              sat = 1;
              tmpres16U = 0xFFFF;
            } 
          }

          qlresult |= ((uint32_t)tmpres16U << 16);

          src16 = quadhigh2 & 0xFFFF;
          tmp8S = (int8_t)(quadhigh & 0xFFFF); 
          if (tmp8S >= 16) { 
            if (src16) { 
              sat = 1;
              qhresult = 0xFFFF; 
            } 
            else { 
              qhresult = 0; 
            } 
          } 
          else if (tmp8S < -16) { 
            qhresult = 0; 
          } 
          else if (tmp8S == -16) { 
            qhresult = src16 >> 15; 
          } 
          else if (tmp8S < 0) { 
            qhresult = (src16 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            qhresult = src16 << tmp8S; 
            if ((qhresult >> tmp8S) != src16) { 
              sat = 1;
              qhresult = 0xFFFF;
            } 
          }

          src16 = (high2>>16) & 0xFFFF;
          tmp8S = (int8_t)((high>>16) & 0xFFFF); 
          if (tmp8S >= 16) { 
            if (src16) { 
              sat = 1;
              tmpres16U = 0xFFFF; 
            } 
            else { 
              tmpres16U = 0; 
            } 
          } 
          else if (tmp8S < -16) { 
            tmpres16U = 0; 
          } 
          else if (tmp8S == -16) { 
            tmpres16U = src16 >> 15; 
          } 
          else if (tmp8S < 0) { 
            tmpres16U = (src16 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          } 
          else { 
            tmpres16U = src16 << tmp8S; 
            if ((tmpres16U >> tmp8S) != src16) { 
              sat = 1;
              tmpres16U = 0xFFFF;
            } 
          }

          qhresult |= ((uint32_t)tmpres16U << 16);
        }
      } //end unsigned
      else { //signed
        tmp8S = (int8_t)(low & 0xFFFF); 
        if (tmp8S < 0) { 
          tmpres16S = ((low2 & 0xFFFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres16S = (low2 & 0xFFFF) << tmp8S; 
          if ((tmpres16S >> tmp8S) != (int16_t)(low2 & 0xFFFF)) { 
            sat = 1;
            tmpres16S = (uint32_t)(1 << (16 - 1)) - ((low2 & 0xFFFF) > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)((low>>16) & 0xFFFF); 
        if (tmp8S < 0) { 
          tmpres16S2 = (((low2>>16) & 0xFFFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres16S2 = ((low2>>16) & 0xFFFF) << tmp8S; 
          if ((tmpres16S2 >> tmp8S) != (int16_t)((low2>>16) & 0xFFFF)) { 
            sat = 1;
            tmpres16S2 = (uint32_t)(1 << (16 - 1)) - (((low2>>16) & 0xFFFF) > 0 ? 1 : 0); 
          } 
        }

        lresult = ((uint32_t)tmpres16S2 << 16) | tmpres16S;

        tmp8S = (int8_t)(high & 0xFFFF); 
        if (tmp8S < 0) { 
          tmpres16S = ((high2 & 0xFFFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres16S = (high2 & 0xFFFF) << tmp8S; 
          if ((tmpres16S >> tmp8S) != (int16_t)(high2 & 0xFFFF)) { 
            sat = 1;
            tmpres16S = (uint32_t)(1 << (16 - 1)) - ((high2 & 0xFFFF) > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)((high>>16) & 0xFFFF); 
        if (tmp8S < 0) { 
          tmpres16S2 = (((high2>>16) & 0xFFFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          tmpres16S2 = ((high2>>16) & 0xFFFF) << tmp8S; 
          if ((tmpres16S2 >> tmp8S) != (int16_t)((high2>>16) & 0xFFFF)) { 
            sat = 1;
            tmpres16S2 = (uint32_t)(1 << (16 - 1)) - (((high2>>16) & 0xFFFF) > 0 ? 1 : 0); 
          } 
        }

        hresult = ((uint32_t)tmpres16S2 << 16) | tmpres16S;

        if(quad) {
          tmp8S = (int8_t)(quadlow & 0xFFFF); 
          if (tmp8S < 0) { 
            tmpres16S = ((quadlow2 & 0xFFFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres16S = (quadlow2 & 0xFFFF) << tmp8S; 
            if ((tmpres16S >> tmp8S) != (int16_t)(quadlow2 & 0xFFFF)) { 
              sat = 1;
              tmpres16S = (uint32_t)(1 << (16 - 1)) - ((quadlow2 & 0xFFFF) > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)((quadlow>>16) & 0xFFFF); 
          if (tmp8S < 0) { 
            tmpres16S2 = (((quadlow2>>16) & 0xFFFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres16S2 = ((quadlow2>>16) & 0xFFFF) << tmp8S; 
            if ((tmpres16S2 >> tmp8S) != (int16_t)((quadlow2>>16) & 0xFFFF)) { 
              sat = 1;
              tmpres16S2 = (uint32_t)(1 << (16 - 1)) - (((quadlow2>>16) & 0xFFFF) > 0 ? 1 : 0); 
            } 
          }

          qlresult = ((uint32_t)tmpres16S2 << 16) | tmpres16S;

          tmp8S = (int8_t)(quadhigh & 0xFFFF); 
          if (tmp8S < 0) { 
            tmpres16S = ((quadhigh2 & 0xFFFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres16S = (quadhigh2 & 0xFFFF) << tmp8S; 
            if ((tmpres16S >> tmp8S) != (int16_t)(quadhigh2 & 0xFFFF)) { 
              sat = 1;
              tmpres16S = (uint32_t)(1 << (16 - 1)) - ((quadhigh2 & 0xFFFF) > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)((quadhigh>>16) & 0xFFFF); 
          if (tmp8S < 0) { 
            tmpres16S2 = (((quadhigh2>>16) & 0xFFFF) + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            tmpres16S2 = ((quadhigh2>>16) & 0xFFFF) << tmp8S; 
            if ((tmpres16S2 >> tmp8S) != (int16_t)((quadhigh2>>16) & 0xFFFF)) { 
              sat = 1;
              tmpres16S2 = (uint32_t)(1 << (16 - 1)) - (((quadhigh2>>16) & 0xFFFF) > 0 ? 1 : 0); 
            } 
          }

          qhresult = ((uint32_t)tmpres16S2 << 16) | tmpres16S;
        }
      } //end signed
      break;
    case 2: //32-bit
      if(is_unsigned) { //unsigned
        shift = (int8_t)low;
        if (shift < 0) {
          lresult = ((uint64_t)low2 + (1 << (-1 - shift))) >> -shift;
        }
        else {
          tmp32U = low2;
          lresult = low2 << shift;
          if ((lresult >> shift) != tmp32U) {
            sat = 1;
            lresult = ~0;
          }
        }

        shift = (int8_t)high;
        if (shift < 0) {
          hresult = ((uint64_t)high2 + (1 << (-1 - shift))) >> -shift;
        }
        else {
          tmp32U = high2;
          hresult = high2 << shift;
          if ((hresult >> shift) != tmp32U) {
            sat = 1;
            hresult = ~0;
          }
        }

        if(quad) {
          shift = (int8_t)quadlow;
          if (shift < 0) {
            qlresult = ((uint64_t)quadlow2 + (1 << (-1 - shift))) >> -shift;
          }
          else {
            tmp32U = quadlow2;
            qlresult = quadlow2 << shift;
            if ((qlresult >> shift) != tmp32U) {
              sat = 1;
              qlresult = ~0;
            }
          }

          shift = (int8_t)quadhigh;
          if (shift < 0) {
            qhresult = ((uint64_t)quadhigh2 + (1 << (-1 - shift))) >> -shift;
          }
          else {
            tmp32U = quadhigh2;
            qhresult = quadhigh2 << shift;
            if ((qhresult >> shift) != tmp32U) {
              sat = 1;
              qhresult = ~0;
            }
          }
        }
      } //end unsigned
      else { //signed
        tmp8S = (int8_t)low; 
        if (tmp8S < 0) { 
          lresult = (low2 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          lresult = low2 << tmp8S; 
          if ((lresult >> tmp8S) != low2) { 
            sat = 1;
            lresult = (uint32_t)(1 << (32 - 1)) - (low2 > 0 ? 1 : 0); 
          } 
        }

        tmp8S = (int8_t)high; 
        if (tmp8S < 0) { 
          hresult = (high2 + (1 << (-1 - tmp8S))) >> -tmp8S; 
        }
        else { 
          hresult = high2 << tmp8S; 
          if ((hresult >> tmp8S) != high2) { 
            sat = 1;
            hresult = (uint32_t)(1 << (32 - 1)) - (high2 > 0 ? 1 : 0); 
          } 
        }

        if(quad) {
          tmp8S = (int8_t)quadlow; 
          if (tmp8S < 0) { 
            qlresult = (quadlow2 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            qlresult = quadlow2 << tmp8S; 
            if ((qlresult >> tmp8S) != quadlow2) { 
              sat = 1;
              qlresult = (uint32_t)(1 << (32 - 1)) - (quadlow2 > 0 ? 1 : 0); 
            } 
          }

          tmp8S = (int8_t)quadhigh; 
          if (tmp8S < 0) { 
            qhresult = (quadhigh2 + (1 << (-1 - tmp8S))) >> -tmp8S; 
          }
          else { 
            qhresult = quadhigh2 << tmp8S; 
            if ((qhresult >> tmp8S) != quadhigh2) { 
              sat = 1;
              qhresult = (uint32_t)(1 << (32 - 1)) - (quadhigh2 > 0 ? 1 : 0); 
            } 
          }
        }
      } //end signed
      break;
    case 4: //64-bit
      if(is_unsigned) { //unsigned
        shift = (int8_t)low;
        valU = ((uint64_t) high2 << 32) | low2;

        if (shift < 0) {
          valU = (valU + (1 << (-1 - shift))) >> -shift;
        }
        else {
          tmpU = valU;
          valU <<= shift;
          if ((valU >> shift) != tmpU) {
            sat = 1;
            valU = ~0;
          }
        }

        lresult = valU & 0xFFFFFFFF;
        hresult = (valU>>32) & 0xFFFFFFFF;

        if(quad) {
          shift = (int8_t)quadlow;
          valU = ((uint64_t) high2 << 32) | low2;

          if (shift < 0) {
            valU = (valU + (1 << (-1 - shift))) >> -shift;
          }
          else {
            tmpU = valU;
            valU <<= shift;
            if ((valU >> shift) != tmpU) {
              sat = 1;
              valU = ~0;
            }
          }

          qlresult = valU & 0xFFFFFFFF;
          qhresult = (valU>>32) & 0xFFFFFFFF;
        }
      } //end unsigned
      else { //signed
        shift = (uint8_t)low;
        valS = ((int64_t)high2 << 32)  | low2;

        if (shift < 0) {
          valS = (valS + (1 << (-1 - shift))) >> -shift;
        }
        else {
          tmpS = valS;
          valS <<= shift;
          if ((valS >> shift) != tmpS) {
            sat = 1;
            valS = tmpS >> 31;
          }
        }

        lresult = valS & 0xFFFFFFFF;
        hresult = (valS>>32) & 0xFFFFFFFF;

        if(quad) {
          shift = (uint8_t)quadlow;
          valS = ((int64_t)quadhigh2 << 32)  | quadlow2;

          if (shift < 0) {
            valS = (valS + (1 << (-1 - shift))) >> -shift;
          }
          else {
            tmpS = valS;
            valS <<= shift;
            if ((valS >> shift) != tmpS) {
              sat = 1;
              valS = tmpS >> 31;
            }
          }

          qlresult = valS & 0xFFFFFFFF;
          qhresult = (valS>>32) & 0xFFFFFFFF;
        }
      } //end signed
      break;
  }

  prog->st32( arg1 + 16, lresult );
  prog->st32( arg1 + 20, hresult );

  if(quad) {
    prog->st32( arg1 + 32, sat);
    prog->st32( arg1 + 24, qlresult );
    prog->st32( arg1 + 28, qhresult );
  }
  else {
    prog->st32( arg1 + 24, sat);
  }

  return 0;
}

uint32_t do_vqdmulh(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low      = prog->ldu32( arg1+4 ) ;
  uint32_t high     = prog->ldu32( arg1+8 ) ;
  uint32_t imm      = prog->ldu32( arg1+12 ) ;

  uint32_t low2;
  uint32_t high2;
  uint32_t quadlow;
  uint32_t quadhigh;
  uint32_t quadlow2;
  uint32_t quadhigh2;

  uint8_t size    = (imm & 0x3);
  uint8_t quad    = (imm >> 2) & 0x1;
  uint8_t round   = (imm>>3) & 0x1;
  uint8_t scalar  = (imm>>4) & 0x1;
  uint8_t scalarV = (imm>>5) & 0x3;

  uint16_t tmpres16;

  uint32_t lresult;
  uint32_t hresult;
  uint32_t qlresult;
  uint32_t qhresult;

  uint32_t tmp32;
  uint64_t tmp64;
  uint8_t sat = 0;

  if(quad) {
    low2        = prog->ldu32( arg1+44 ) ;
    high2       = prog->ldu32( arg1+48 ) ;

    quadlow    = prog->ldu32( arg1+36 ) ;
    quadhigh   = prog->ldu32( arg1+40 ) ;

    quadlow2    = prog->ldu32( arg1+52 ) ;
    quadhigh2   = prog->ldu32( arg1+56 ) ;
  }
  else {
    low2  = prog->ldu32( arg1+28 ) ;
    high2 = prog->ldu32( arg1+32 ) ;
  }


  switch(size) {
    case 1: //16-bit
      if(scalar) {
        switch(scalarV) {
          case 0:
            tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
            break;
          case 1:
            tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
            break;
          case 2:
            tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
            break;
          case 3:
            tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
            break;
        }
      }
      else
        tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) (low2 & 0xFFFF);

      if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
        sat = 1;
        tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
      }
      else { 
        tmp32 <<= 1; 
      } 

      if (round) { 
        int32_t old = tmp32; 
        tmp32 += 1 << 15; 
        if ((int32_t)tmp32 < old) { 
          sat = 1;
          tmp32 = SIGNBIT - 1; 
        } 
      } 

      tmpres16 = (tmp32 >> 16) & 0xFFFF; 

      if(scalar) {
        switch(scalarV) {
          case 0:
            tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
            break;
          case 1:
            tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
            break;
          case 2:
            tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
            break;
          case 3:
            tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
            break;
        }
      }
      else
        tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);

      if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
        sat = 1;
        tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
      }
      else { 
        tmp32 <<= 1; 
      } 

      if (round) { 
        int32_t old = tmp32; 
        tmp32 += 1 << 15; 
        if ((int32_t)tmp32 < old) { 
          sat = 1;
          tmp32 = SIGNBIT - 1; 
        } 
      } 

      lresult = (tmp32 >> 16) & 0xFFFF; 
      lresult <<= 16;
      lresult |= tmpres16;

      if(scalar) {
        switch(scalarV) {
          case 0:
            tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
            break;
          case 1:
            tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
            break;
          case 2:
            tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
            break;
          case 3:
            tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
            break;
        }
      }
      else 
        tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) (high2 & 0xFFFF);

      if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
        sat = 1;
        tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
      }
      else { 
        tmp32 <<= 1; 
      } 

      if (round) { 
        int32_t old = tmp32; 
        tmp32 += 1 << 15; 
        if ((int32_t)tmp32 < old) { 
          sat = 1;
          tmp32 = SIGNBIT - 1; 
        } 
      } 

      tmpres16 = (tmp32 >> 16) & 0xFFFF; 

      if(scalar) {
        switch(scalarV) {
          case 0:
            tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
            break;
          case 1:
            tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
            break;
          case 2:
            tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
            break;
          case 3:
            tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
            break;
        }
      }
      else 
        tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);

      if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
        sat = 1;
        tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
      }
      else { 
        tmp32 <<= 1; 
      } 

      if (round) { 
        int32_t old = tmp32; 
        tmp32 += 1 << 15; 
        if ((int32_t)tmp32 < old) { 
          sat = 1;
          tmp32 = SIGNBIT - 1; 
        } 
      } 

      hresult = (tmp32 >> 16) & 0xFFFF; 
      hresult <<= 16;
      hresult |= tmpres16;

      if(quad) {
        if(scalar) {
          switch(scalarV) {
            case 0:
              tmp32 = (int32_t)(int16_t) (quadlow & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
              break;
            case 1:
              tmp32 = (int32_t)(int16_t) (quadlow & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
              break;
            case 2:
              tmp32 = (int32_t)(int16_t) (quadlow & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
              break;
            case 3:
              tmp32 = (int32_t)(int16_t) (quadlow & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
              break;
          }
        }
        else
          tmp32 = (int32_t)(int16_t) (quadlow & 0xFFFF) * (int16_t) (quadlow2 & 0xFFFF);

        if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
          sat = 1;
          tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
        }
        else { 
          tmp32 <<= 1; 
        } 

        if (round) { 
          int32_t old = tmp32; 
          tmp32 += 1 << 15; 
          if ((int32_t)tmp32 < old) { 
            sat = 1;
            tmp32 = SIGNBIT - 1; 
          } 
        } 

        tmpres16 = (tmp32 >> 16) & 0xFFFF; 

        if(scalar) {
          switch(scalarV) {
            case 0:
              tmp32 = (int32_t)(int16_t) ((quadlow>>16) & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
              break;
            case 1:
              tmp32 = (int32_t)(int16_t) ((quadlow>>16) & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
              break;
            case 2:
              tmp32 = (int32_t)(int16_t) ((quadlow>>16) & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
              break;
            case 3:
              tmp32 = (int32_t)(int16_t) ((quadlow>>16) & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
              break;
          }
        }
        else
          tmp32 = (int32_t)(int16_t) ((quadlow>>16) & 0xFFFF) * (int16_t) ((quadlow2>>16) & 0xFFFF);

        if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
          sat = 1;
          tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
        }
        else { 
          tmp32 <<= 1; 
        } 

        if (round) { 
          int32_t old = tmp32; 
          tmp32 += 1 << 15; 
          if ((int32_t)tmp32 < old) { 
            sat = 1;
            tmp32 = SIGNBIT - 1; 
          } 
        } 

        qlresult = (tmp32 >> 16) & 0xFFFF; 
        qlresult <<= 16;
        qlresult |= tmpres16;

        if(scalar) {
          switch(scalarV) {
            case 0:
              tmp32 = (int32_t)(int16_t) (quadhigh & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
              break;
            case 1:
              tmp32 = (int32_t)(int16_t) (quadhigh & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
              break;
            case 2:
              tmp32 = (int32_t)(int16_t) (quadhigh & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
              break;
            case 3:
              tmp32 = (int32_t)(int16_t) (quadhigh & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
              break;
          }
        }
        else 
          tmp32 = (int32_t)(int16_t) (quadhigh & 0xFFFF) * (int16_t) (quadhigh2 & 0xFFFF);

        if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
          sat = 1;
          tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
        }
        else { 
          tmp32 <<= 1; 
        } 

        if (round) { 
          int32_t old = tmp32; 
          tmp32 += 1 << 15; 
          if ((int32_t)tmp32 < old) { 
            sat = 1;
            tmp32 = SIGNBIT - 1; 
          } 
        } 

        tmpres16 = (tmp32 >> 16) & 0xFFFF; 

        if(scalar) {
          switch(scalarV) {
            case 0:
              tmp32 = (int32_t)(int16_t) ((quadhigh>>16) & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
              break;
            case 1:
              tmp32 = (int32_t)(int16_t) ((quadhigh>>16) & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
              break;
            case 2:
              tmp32 = (int32_t)(int16_t) ((quadhigh>>16) & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
              break;
            case 3:
              tmp32 = (int32_t)(int16_t) ((quadhigh>>16) & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
              break;
          }
        }
        else
          tmp32 = (int32_t)(int16_t) ((quadhigh>>16) & 0xFFFF) * (int16_t) ((quadhigh2>>16) & 0xFFFF);

        if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
          sat = 1;
          tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
        }
        else { 
          tmp32 <<= 1; 
        } 

        if (round) { 
          int32_t old = tmp32; 
          tmp32 += 1 << 15; 
          if ((int32_t)tmp32 < old) { 
            sat = 1;
            tmp32 = SIGNBIT - 1; 
          } 
        } 

        qhresult = (tmp32 >> 16) & 0xFFFF; 
        qhresult <<= 16;
        qhresult |= tmpres16;
      }
      break;
    case 2: //32-bit
      if(scalar) {
        if(scalarV)
          tmp64 = (int64_t)(int32_t) low * (int32_t) high2; 
        else
          tmp64 = (int64_t)(int32_t) low * (int32_t) low2; 
      }
      else 
        tmp64 = (int64_t)(int32_t) low * (int32_t) low2; 

      if ((tmp64 ^ (tmp64 << 1)) & SIGNBIT64) { 
        sat = 1;
        tmp64 = (tmp64 >> 63) ^ ~SIGNBIT64; 
      }
      else { 
        tmp64 <<= 1; 
      } 

      if (round) { 
        int64_t old = tmp64; 
        tmp64 += (int64_t)1 << 31; 
        if ((int64_t)tmp64 < old) { 
          sat = 1;
          tmp64 = SIGNBIT64 - 1; 
        } 
      } 

      lresult = tmp64 >> 32;

      if(scalar) {
        if(scalarV)
          tmp64 = (int64_t)(int32_t) high * (int32_t) high2; 
        else
          tmp64 = (int64_t)(int32_t) high * (int32_t) low2; 
      }
      else
        tmp64 = (int64_t)(int32_t) high * (int32_t) high2; 

      if ((tmp64 ^ (tmp64 << 1)) & SIGNBIT64) { 
        sat = 1;
        tmp64 = (tmp64 >> 63) ^ ~SIGNBIT64; 
      }
      else { 
        tmp64 <<= 1; 
      } 

      if (round) { 
        int64_t old = tmp64; 
        tmp64 += (int64_t)1 << 31; 
        if ((int64_t)tmp64 < old) { 
          sat = 1;
          tmp64 = SIGNBIT64 - 1; 
        } 
      } 

      hresult = tmp64 >> 32;

      if(quad) {
        if(scalar) {
          if(scalarV)
            tmp64 = (int64_t)(int32_t) quadlow * (int32_t) high2; 
          else
            tmp64 = (int64_t)(int32_t) quadlow * (int32_t) low2; 
        }
        else
          tmp64 = (int64_t)(int32_t) quadlow * (int32_t) quadlow2; 

        if ((tmp64 ^ (tmp64 << 1)) & SIGNBIT64) { 
          sat = 1;
          tmp64 = (tmp64 >> 63) ^ ~SIGNBIT64; 
        }
        else { 
          tmp64 <<= 1; 
        } 

        if (round) { 
          int64_t old = tmp64; 
          tmp64 += (int64_t)1 << 31; 
          if ((int64_t)tmp64 < old) { 
            sat = 1;
            tmp64 = SIGNBIT64 - 1; 
          } 
        } 

        qlresult = tmp64 >> 32;

        if(scalar) {
          if(scalarV)
            tmp64 = (int64_t)(int32_t) quadhigh * (int32_t) high2; 
          else
            tmp64 = (int64_t)(int32_t) quadhigh * (int32_t) low2; 
        }
        else
          tmp64 = (int64_t)(int32_t) quadhigh * (int32_t) quadhigh2; 

        if ((tmp64 ^ (tmp64 << 1)) & SIGNBIT64) { 
          sat = 1;
          tmp64 = (tmp64 >> 63) ^ ~SIGNBIT64; 
        }
        else { 
          tmp64 <<= 1; 
        } 

        if (round) { 
          int64_t old = tmp64; 
          tmp64 += (int64_t)1 << 31; 
          if ((int64_t)tmp64 < old) { 
            sat = 1;
            tmp64 = SIGNBIT64 - 1; 
          } 
        } 

        qhresult = tmp64 >> 32;
      }
      break;
  }

  prog->st32( arg1 + 16, lresult );
  prog->st32( arg1 + 20, hresult );

  if(quad) {
    prog->st32( arg1 + 32, sat);
    prog->st32( arg1 + 24, qlresult );
    prog->st32( arg1 + 28, qhresult );
  }
  else {
    prog->st32( arg1 + 24, sat);
  }

  return 0;
}

uint32_t do_vqdmlal_sl(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low      = prog->ldu32( arg1+4 );
  uint32_t high     = prog->ldu32( arg1+8 );
  uint32_t imm      = prog->ldu32( arg1+12 );

  uint32_t low2     = prog->ldu32( arg1+44 );
  uint32_t high2    = prog->ldu32( arg1+48 );

  uint32_t acclow1  = prog->ldu32( arg1+16 );
  uint32_t acchigh1 = prog->ldu32( arg1+20 );

  uint32_t acclow2  = prog->ldu32( arg1+24 );
  uint32_t acchigh2 = prog->ldu32( arg1+28 );

  uint8_t size    = (imm & 0x3);
  uint8_t op      = (imm>>3) & 0x1;
  uint8_t scalar  = (imm>>4) & 0x1;
  uint8_t scalarV = (imm>>5) & 0x3;

  uint32_t lresult;
  uint32_t hresult;
  uint32_t qlresult;
  uint32_t qhresult;

  uint32_t tmp32;
  uint64_t tmp64;
  uint8_t sat = 0;

  uint64_t result;
  uint32_t x, y;

  switch(size) {
    case 1: //16-bit
      if(scalar) {
        switch(scalarV) {
          case 0:
            tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
            break;
          case 1:
            tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
            break;
          case 2:
            tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
            break;
          case 3:
            tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
            break;
        }
      }
      else
        tmp32 = (int32_t)(int16_t) (low & 0xFFFF) * (int16_t) (low2 & 0xFFFF);

      if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
        sat = 1;
        tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
      }
      else
        tmp32 <<= 1; 

      if(!op) {
        x = acclow1;
        y = tmp32;
        lresult = x + y;
        if (((lresult ^ x) & SIGNBIT) && !((x ^ y) & SIGNBIT)) {
          sat = 1;
          lresult = ((int32_t)x >> 31) ^ ~SIGNBIT;
        }
      }
      else {
        lresult = acclow1 - tmp32;
        if (((lresult ^ acclow1) & SIGNBIT) && ((acclow1 ^ tmp32) & SIGNBIT)) {
            sat = 1;
            lresult = ~(((int32_t)acclow1 >> 31) ^ SIGNBIT);
        }
      }
      
      if(scalar) {
        switch(scalarV) {
          case 0:
            tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
            break;
          case 1:
            tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
            break;
          case 2:
            tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
            break;
          case 3:
            tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
            break;
        }
      }
      else
        tmp32 = (int32_t)(int16_t) ((low>>16) & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);

      if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
        sat = 1;
        tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
      }
      else 
        tmp32 <<= 1; 

      if(!op) {
        x = acchigh1;
        y = tmp32;
        hresult = x + y;
        if (((hresult ^ x) & SIGNBIT) && !((x ^ y) & SIGNBIT)) {
          sat = 1;
          hresult = ((int32_t)x >> 31) ^ ~SIGNBIT;
        }
      }
      else {
        hresult = acchigh1 - tmp32;
        if (((hresult ^ acchigh1) & SIGNBIT) && ((acchigh1 ^ tmp32) & SIGNBIT)) {
            sat = 1;
            hresult = ~(((int32_t)acchigh1 >> 31) ^ SIGNBIT);
        }
      }
      
      if(scalar) {
        switch(scalarV) {
          case 0:
            tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
            break;
          case 1:
            tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
            break;
          case 2:
            tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
            break;
          case 3:
            tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
            break;
        }
      }
      else 
        tmp32 = (int32_t)(int16_t) (high & 0xFFFF) * (int16_t) (high2 & 0xFFFF);

      if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
        sat = 1;
        tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
      }
      else
        tmp32 <<= 1; 

      if(!op) {
        x = acclow2;
        y = tmp32;
        qlresult = x + y;
        if (((qlresult ^ x) & SIGNBIT) && !((x ^ y) & SIGNBIT)) {
          sat = 1;
          qlresult = ((int32_t)x >> 31) ^ ~SIGNBIT;
        }
      }
      else {
        qlresult = acclow2 - tmp32;
        if (((qlresult ^ acclow2) & SIGNBIT) && ((acclow2 ^ tmp32) & SIGNBIT)) {
            sat = 1;
            qlresult = ~(((int32_t)acclow2 >> 31) ^ SIGNBIT);
        }
      }
      
      if(scalar) {
        switch(scalarV) {
          case 0:
            tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) (low2 & 0xFFFF);
            break;
          case 1:
            tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) ((low2>>16) & 0xFFFF);
            break;
          case 2:
            tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) (high2 & 0xFFFF);
            break;
          case 3:
            tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);
            break;
        }
      }
      else 
        tmp32 = (int32_t)(int16_t) ((high>>16) & 0xFFFF) * (int16_t) ((high2>>16) & 0xFFFF);

      if ((tmp32 ^ (tmp32 << 1)) & SIGNBIT) { 
        sat = 1;
        tmp32 = (tmp32 >> 31) ^ ~SIGNBIT; 
      }
      else
        tmp32 <<= 1; 

      if(!op) {
        x = acchigh2;
        y = tmp32;
        qhresult = x + y;
        if (((qhresult ^ x) & SIGNBIT) && !((x ^ y) & SIGNBIT)) {
          sat = 1;
          qhresult = ((int32_t)x >> 31) ^ ~SIGNBIT;
        }
      }
      else {
        qhresult = acchigh2 - tmp32;
        if (((qhresult ^ acchigh2) & SIGNBIT) && ((acchigh2 ^ tmp32) & SIGNBIT)) {
            sat = 1;
            qhresult = ~(((int32_t)acchigh2 >> 31) ^ SIGNBIT);
        }
      }
      break;
    case 2: //32-bit
      uint64_t acc1 = ((uint64_t)acchigh1 << 32) | acclow1;
      uint64_t acc2 = ((uint64_t)acchigh2 << 32) | acclow2;

      if(scalar) {
        if(scalarV)
          tmp64 = (int64_t)(int32_t) low * (int32_t) high2; 
        else
          tmp64 = (int64_t)(int32_t) low * (int32_t) low2; 
      }
      else 
        tmp64 = (int64_t)(int32_t) low * (int32_t) low2; 

      if ((tmp64 ^ (tmp64 << 1)) & SIGNBIT64) { 
        sat = 1;
        tmp64 = (tmp64 >> 63) ^ ~SIGNBIT64; 
      }
      else
        tmp64 <<= 1; 

      if(!op) {
        result = acc1 + tmp64;
        if (((result ^ acc1) & SIGNBIT64) && !((acc1 ^ tmp64) & SIGNBIT64)) {
          sat = 1;
          result = ((int64_t)acc1 >> 63) ^ ~SIGNBIT64;
        }
      }
      else {
        result = acc1 - tmp64;
        if (((result ^ acc1) & SIGNBIT64) && ((acc1 ^ tmp64) & SIGNBIT64)) {
          sat = 1;
          result = ~((int64_t)acc1 >> 63) ^ SIGNBIT64;
        }
      }

      lresult = result & 0xFFFFFFFF;
      hresult = result >> 32;

      if(scalar) {
        if(scalarV)
          tmp64 = (int64_t)(int32_t) high * (int32_t) high2; 
        else
          tmp64 = (int64_t)(int32_t) high * (int32_t) low2; 
      }
      else
        tmp64 = (int64_t)(int32_t) high * (int32_t) high2; 

      if ((tmp64 ^ (tmp64 << 1)) & SIGNBIT64) { 
        sat = 1;
        tmp64 = (tmp64 >> 63) ^ ~SIGNBIT64; 
      }
      else
        tmp64 <<= 1; 

      if(!op) {
        result = acc2 + tmp64;
        if (((result ^ acc2) & SIGNBIT64) && !((acc2 ^ tmp64) & SIGNBIT64)) {
          sat = 1;
          result = ~((int64_t)acc2 >> 63) ^ SIGNBIT64;
        }
      }
      else {
        result = acc2 - tmp64;
        if (((result ^ acc2) & SIGNBIT64) && ((acc2 ^ tmp64) & SIGNBIT64)) {
          sat = 1;
          result = ~((int64_t)acc2 >> 63) ^ SIGNBIT64;
        }
      }

      qlresult = result & 0xFFFFFFFF;
      qhresult = result >> 32;
      break;
  }

  prog->st32( arg1 + 16, lresult );
  prog->st32( arg1 + 20, hresult );
  prog->st32( arg1 + 32, sat);
  prog->st32( arg1 + 24, qlresult );
  prog->st32( arg1 + 28, qhresult );

  return 0;
}

uint32_t do_vmulpoly(ProgramBase *prog, uint32_t arg1)
{
  uint32_t op1L = prog->ldu32( arg1+4 );
  uint32_t op1H = prog->ldu32( arg1+8 );
  uint32_t op1Lq;
  uint32_t op1Hq;

  uint32_t op2L;
  uint32_t op2H;
  uint32_t op2Lq;
  uint32_t op2Hq;

  uint32_t imm = prog->ldu32( arg1+12 );


  uint32_t lresult;
  uint32_t hresult;

  uint32_t qlresult;
  uint32_t qhresult;

  //VMUL
  uint32_t mask;

  //VMULL
  uint64_t result = 0;
  uint64_t hmask;
  uint64_t op2ex;

  uint8_t vmul = imm & 1;
  uint8_t quad = (imm&2)>>1;

  if(quad) {
    op2L = prog->ldu32( arg1+44 ) ;
    op2H = prog->ldu32( arg1+48 ) ;

    op1Lq = prog->ldu32( arg1+36 ) ;
    op1Hq = prog->ldu32( arg1+40 ) ;

    op2Lq = prog->ldu32( arg1+52 ) ;
    op2Hq = prog->ldu32( arg1+56 ) ;
  }
  else {
    op2L = prog->ldu32( arg1+28 ) ;
    op2H = prog->ldu32( arg1+32 ) ;
  }

  if(vmul) {
    lresult = 0;
    hresult = 0;

    while (op1L) {
      mask = 0;
      
      if (op1L & 1)
        mask |= 0xff;

      if (op1L & (1 << 8))
        mask |= (0xff << 8);

      if (op1L & (1 << 16))
        mask |= (0xff << 16);

      if (op1L & (1 << 24))
        mask |= (0xff << 24);

      lresult ^= op2L & mask;

      op1L = (op1L >> 1) & 0x7f7f7f7f;
      op2L = (op2L << 1) & 0xfefefefe;
    }
    
    while (op1H) {
      mask = 0;
      
      if (op1H & 1)
        mask |= 0xff;

      if (op1H & (1 << 8))
        mask |= (0xff << 8);

      if (op1H & (1 << 16))
        mask |= (0xff << 16);

      if (op1H & (1 << 24))
        mask |= (0xff << 24);

      hresult ^= op2H & mask;

      op1H = (op1H >> 1) & 0x7f7f7f7f;
      op2H = (op2H << 1) & 0xfefefefe;
    }

    prog->st32( arg1 + 16, lresult );
    prog->st32( arg1 + 20, hresult );

    if(quad) {
      qlresult = 0;
      qhresult = 0;

      while (op1Lq) {
        mask = 0;
        
        if (op1Lq & 1)
          mask |= 0xff;

        if (op1Lq & (1 << 8))
          mask |= (0xff << 8);

        if (op1Lq & (1 << 16))
          mask |= (0xff << 16);

        if (op1Lq & (1 << 24))
          mask |= (0xff << 24);

        qlresult ^= op2Lq & mask;

        op1Lq = (op1Lq >> 1) & 0x7f7f7f7f;
        op2Lq = (op2Lq << 1) & 0xfefefefe;
      }
      
      while (op1Hq) {
        mask = 0;
        
        if (op1Hq & 1)
          mask |= 0xff;

        if (op1Hq & (1 << 8))
          mask |= (0xff << 8);

        if (op1Hq & (1 << 16))
          mask |= (0xff << 16);

        if (op1Hq & (1 << 24))
          mask |= (0xff << 24);

        qhresult ^= op2Hq & mask;

        op1Hq = (op1Hq >> 1) & 0x7f7f7f7f;
        op2Hq = (op2Hq << 1) & 0xfefefefe;
      }

      prog->st32( arg1 + 24, qlresult );
      prog->st32( arg1 + 28, qhresult );
    }
  }
  else { //vmull //long destination
    op2ex = op2L;

    op2ex = (op2ex & 0xff) |
        ((op2ex & 0xff00) << 8) |
        ((op2ex & 0xff0000) << 16) |
        ((op2ex & 0xff000000) << 24);

    while (op1L) {
      hmask = 0;

      if (op1L & 1) {
        hmask |= 0xffff;
      }

      if (op1L & (1 << 8)) {
        hmask |= (0xffffU << 16);
      }

      if (op1L & (1 << 16)) {
        hmask |= (0xffffULL << 32);
      }

      if (op1L & (1 << 24)) {
        hmask |= (0xffffULL << 48);
      }

      result ^= op2ex & hmask;
      op1L = (op1L >> 1) & 0x7f7f7f7f;
      op2ex <<= 1;
    }

    lresult = result & 0xFFFFFFFF;
    hresult = result >> 32;

    prog->st32( arg1 + 16, lresult );
    prog->st32( arg1 + 20, hresult );

    op2ex = op2H;

    op2ex = (op2ex & 0xff) |
        ((op2ex & 0xff00) << 8) |
        ((op2ex & 0xff0000) << 16) |
        ((op2ex & 0xff000000) << 24);

    result = 0;

    while (op1H) {
      hmask = 0;

      if (op1H & 1) {
        hmask |= 0xffff;
      }

      if (op1H & (1 << 8)) {
        hmask |= (0xffffU << 16);
      }

      if (op1H & (1 << 16)) {
        hmask |= (0xffffULL << 32);
      }

      if (op1H & (1 << 24)) {
        hmask |= (0xffffULL << 48);
      }

      result ^= op2ex & hmask;
      op1H = (op1H >> 1) & 0x7f7f7f7f;
      op2ex <<= 1;
    }

    qlresult = result & 0xFFFFFFFF;
    qhresult = result >> 32;

    prog->st32( arg1 + 24, qlresult );
    prog->st32( arg1 + 28, qhresult );
  }

  return 0;
}

uint32_t do_vqshrn(ProgramBase *prog, uint32_t arg1)
{
  uint32_t low   = prog->ldu32( arg1+4 ) ;
  uint32_t high  = prog->ldu32( arg1+8 ) ;
  uint32_t imm   = prog->ldu32( arg1+12 ) ;

  uint32_t qlow  = prog->ldu32( arg1+36 ) ;
  uint32_t qhigh = prog->ldu32( arg1+40 ) ;

  uint8_t size = (imm & 0x3);
  uint8_t round = (imm >> 2) & 0x1;
  uint8_t is_unsigned = (imm>>3) & 0x1;
  uint8_t shiftop = (imm>>4) & 0x3F;
  uint8_t op = (imm>>10) & 0x1;

  uint16_t tmpres16U, tmpres16U2;
  uint32_t tmpres32U, tmpres32U2;
  uint64_t valU, valU2;

  uint8_t sat = 0;

  uint32_t high1,low1;
  int32_t highS1,lowS1;

  uint8_t input_unsigned = (op == 0) ? !is_unsigned : is_unsigned;

  switch(size) {
    case 1: //16-bit
      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16 || tmp < -16) { 
              tmpres16U = 0; 
          } 
          else if (tmp == -16) { 
            tmpres16U = low >> (-tmp - 1); 
          }
          else if (tmp < 0) { 
            tmpres16U = (low + (1 << (-1 - tmp))) >> -tmp; 
          }
          else { 
            tmpres16U = low << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if ((tmp >= 16) || (tmp <= -16)) { 
            tmpres16U = 0; 
          }
          else if (tmp < 0) { 
            tmpres16U = (low + (1 << (-1 - tmp))) >> -tmp; 
          }
          else { 
            tmpres16U = low << tmp; 
          }
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16 || tmp <= -16) { 
            tmpres16U = 0; 
          } else if (tmp < 0) { 
            tmpres16U = low >> -tmp; 
          } else { 
            tmpres16U = low << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16) { 
            tmpres16U = 0; 
          } 
          else if (tmp <= -16) { 
            tmpres16U = low >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres16U = low >> -tmp; 
          } 
          else { 
            tmpres16U = low << tmp; 
          }
        }
      }

      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16 || tmp < -16) { 
              tmpres16U2 = 0; 
          } 
          else if (tmp == -16) { 
            tmpres16U2 = high >> (-tmp - 1); 
          }
          else if (tmp < 0) { 
            tmpres16U2 = (high + (1 << (-1 - tmp))) >> -tmp; 
          }
          else { 
            tmpres16U2 = high << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if ((tmp >= 16) || (tmp <= -16)) { 
            tmpres16U2 = 0; 
          }
          else if (tmp < 0) { 
            tmpres16U2 = (high + (1 << (-1 - tmp))) >> -tmp; 
          }
          else { 
            tmpres16U2 = high << tmp; 
          }
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16 || tmp <= -16) { 
            tmpres16U2 = 0; 
          } else if (tmp < 0) { 
            tmpres16U2 = high >> -tmp; 
          } else { 
            tmpres16U2 = high << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16) { 
            tmpres16U2 = 0; 
          } 
          else if (tmp <= -16) { 
            tmpres16U2 = high >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres16U2 = high >> -tmp; 
          } 
          else { 
            tmpres16U2 = high << tmp; 
          }
        }
      }

      valU = ((uint64_t)tmpres16U2 << 32) | tmpres16U;

      //narrow
      if(!op) {
        if(is_unsigned) {
          uint16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = valU;
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d; 
          }
      
          s = valU >> 16;
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 8; 
          }

          s = valU >> 32;
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 16; 
          }

          s = valU >> 48;
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 24; 
          }

          valU = res;
        }
        else {
          valU = (valU & 0xffu) | ((valU >> 8) & 0xff00u) | ((valU >> 16) & 0xff0000u) | ((valU >> 24) & 0xff000000u);
        }
      }
      else {
        if(is_unsigned) {
          uint16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = valU; 
          if (s > 0xff) { 
            d = 0xff; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d;
      
          s = valU >> 16; 
          if (s > 0xff) { 
            d = 0xff; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 8;
      
          s = valU >> 32; 
          if (s > 0xff) { 
            d = 0xff; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 16;
      
          s = valU >> 48; 
          if (s > 0xff) { 
            d = 0xff; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 24;
      
          valU = res;
        }
        else {
          int16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = valU;
          if (s != (int8_t)s) { 
            d = (s >> 15) ^ 0x7f; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d;
      
          s = valU >> 16; 
          if (s != (int8_t)s) { 
            d = (s >> 15) ^ 0x7f; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 8;

          s = valU >> 32; 
          if (s != (int8_t)s) { 
            d = (s >> 15) ^ 0x7f; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 16;

          s = valU >> 48; 
          if (s != (int8_t)s) { 
            d = (s >> 15) ^ 0x7f; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 24;

          valU = res;
        }
      }

      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16 || tmp < -16) { 
              tmpres16U = 0; 
          } 
          else if (tmp == -16) { 
            tmpres16U = qlow >> (-tmp - 1); 
          }
          else if (tmp < 0) { 
            tmpres16U = (qlow + (1 << (-1 - tmp))) >> -tmp; 
          }
          else { 
            tmpres16U = qlow << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if ((tmp >= 16) || (tmp <= -16)) { 
            tmpres16U = 0; 
          }
          else if (tmp < 0) { 
            tmpres16U = (qlow + (1 << (-1 - tmp))) >> -tmp; 
          }
          else { 
            tmpres16U = qlow << tmp; 
          }
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16 || tmp <= -16) { 
            tmpres16U = 0; 
          } else if (tmp < 0) { 
            tmpres16U = qlow >> -tmp; 
          } else { 
            tmpres16U = qlow << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16) { 
            tmpres16U = 0; 
          } 
          else if (tmp <= -16) { 
            tmpres16U = qlow >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres16U = qlow >> -tmp; 
          } 
          else { 
            tmpres16U = qlow << tmp; 
          }
        }
      }

      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16 || tmp < -16) { 
              tmpres16U2 = 0; 
          } 
          else if (tmp == -16) { 
            tmpres16U2 = qhigh >> (-tmp - 1); 
          }
          else if (tmp < 0) { 
            tmpres16U2 = (qhigh + (1 << (-1 - tmp))) >> -tmp; 
          }
          else { 
            tmpres16U2 = qhigh << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if ((tmp >= 16) || (tmp <= -16)) { 
            tmpres16U2 = 0; 
          }
          else if (tmp < 0) { 
            tmpres16U2 = (qhigh + (1 << (-1 - tmp))) >> -tmp; 
          }
          else { 
            tmpres16U2 = qhigh << tmp; 
          }
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16 || tmp <= -16) { 
            tmpres16U2 = 0; 
          } else if (tmp < 0) { 
            tmpres16U2 = qhigh >> -tmp; 
          } else { 
            tmpres16U2 = qhigh << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 16) { 
            tmpres16U2 = 0; 
          } 
          else if (tmp <= -16) { 
            tmpres16U2 = qhigh >> 15; 
          } 
          else if (tmp < 0) { 
            tmpres16U2 = qhigh >> -tmp; 
          } 
          else { 
            tmpres16U2 = qhigh << tmp; 
          }
        }
      }

      valU2 = ((uint64_t)tmpres16U2 << 32) | tmpres16U;

      //narrow
      if(!op) {
        if(is_unsigned) {
          uint16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = valU2;
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d; 
          }
      
          s = valU2 >> 16;
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 8; 
          }

          s = valU2 >> 32;
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 16; 
          }

          s = valU2 >> 48;
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 24; 
          }

          valU2 = res;
        }
        else {
          valU2 = (valU2 & 0xffu) | ((valU2 >> 8) & 0xff00u) | ((valU2 >> 16) & 0xff0000u) | ((valU2 >> 24) & 0xff000000u);
        }
      }
      else {
        if(is_unsigned) {
          uint16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = valU2; 
          if (s > 0xff) { 
            d = 0xff; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d;
      
          s = valU2 >> 16; 
          if (s > 0xff) { 
            d = 0xff; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 8;
      
          s = valU2 >> 32; 
          if (s > 0xff) { 
            d = 0xff; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 16;
      
          s = valU2 >> 48; 
          if (s > 0xff) { 
            d = 0xff; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 24;
      
          valU2 = res;
        }
        else {
          int16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = valU2;
          if (s != (int8_t)s) { 
            d = (s >> 15) ^ 0x7f; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d;
      
          s = valU2 >> 16; 
          if (s != (int8_t)s) { 
            d = (s >> 15) ^ 0x7f; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 8;

          s = valU2 >> 32; 
          if (s != (int8_t)s) { 
            d = (s >> 15) ^ 0x7f; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 16;

          s = valU2 >> 48; 
          if (s != (int8_t)s) { 
            d = (s >> 15) ^ 0x7f; 
            sat = 1;
          } 
          else  { 
            d = s; 
          } 
          res |= (uint32_t)d << 24;

          valU2 = res;
        }
      }

      break;
    case 2: //32-bit
      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t shift = (int8_t)shiftop;
          if (shift >= 32 || shift < -32) {
            tmpres32U = 0;
          } 
          else if (shift == -32) {
            tmpres32U = low >> 31;
          } 
          else if (shift < 0) {
            uint64_t big_tmpres32U = ((uint64_t)low + (1 << (-1 - shift)));
            tmpres32U = big_tmpres32U >> -shift;
          } 
          else {
            tmpres32U = low << shift;
          }
        }
        else {
          int32_t dest;
          int32_t val = (int32_t)low;
          int8_t shift = (int8_t)shiftop;

          if ((shift >= 32) || (shift <= -32)) {
            dest = 0;
          } 
          else if (shift < 0) {
            int64_t big_dest = ((int64_t)val + (1 << (-1 - shift)));
            dest = big_dest >> -shift;
          } 
          else {
            dest = val << shift;
          }
          tmpres32U = dest;
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 32 || tmp <= -32 ) { 
            tmpres32U = 0; 
          }
          else if (tmp < 0) { 
            tmpres32U = low >> -tmp; 
          }
          else { 
            tmpres32U = low << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 32) { 
            tmpres32U = 0; 
          } 
          else if (tmp <= -32) { 
            tmpres32U = low >> 31; 
          } 
          else if (tmp < 0) { 
            tmpres32U = low >> -tmp; 
          } 
          else { 
            tmpres32U = low << tmp; 
          }
        }
      }

      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t shift = (int8_t)shiftop;
          if (shift >= 32 || shift < -32) {
            tmpres32U2 = 0;
          } 
          else if (shift == -32) {
            tmpres32U2 = high >> 31;
          } 
          else if (shift < 0) {
            uint64_t big_tmpres32U2 = ((uint64_t)high + (1 << (-1 - shift)));
            tmpres32U2 = big_tmpres32U2 >> -shift;
          } 
          else {
            tmpres32U2 = high << shift;
          }
        }
        else {
          int32_t dest;
          int32_t val = (int32_t)high;
          int8_t shift = (int8_t)shiftop;

          if ((shift >= 32) || (shift <= -32)) {
            dest = 0;
          } 
          else if (shift < 0) {
            int64_t big_dest = ((int64_t)val + (1 << (-1 - shift)));
            dest = big_dest >> -shift;
          } 
          else {
            dest = val << shift;
          }
          tmpres32U2 = dest;
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 32 || tmp <= -32 ) { 
            tmpres32U2 = 0; 
          }
          else if (tmp < 0) { 
            tmpres32U2 = high >> -tmp; 
          }
          else { 
            tmpres32U2 = high << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 32) { 
            tmpres32U2 = 0; 
          } 
          else if (tmp <= -32) { 
            tmpres32U2 = high >> 31; 
          } 
          else if (tmp < 0) { 
            tmpres32U2 = high >> -tmp; 
          } 
          else { 
            tmpres32U2 = high << tmp; 
          }
        }
      }

      valU = ((uint64_t)tmpres32U2 << 32) | tmpres32U;

      //narrow
      if(!op) {
        if(is_unsigned) {
          low1 = valU;
          if (low1 & 0x80000000) {
            low1 = 0;
            sat = 1;
          }
          else if (low1 > 0xffff) {
            low1 = 0xffff;
            sat = 1;
          }

          high1 = valU >> 32;
          if (high1 & 0x80000000) {
            high1 = 0;
            sat = 1;
          }
          else if (high1 > 0xffff) {
            high1 = 0xffff;
            sat = 1;
          }

          valU = (low1 | (high1 << 16));
        }
        else {
          valU = (valU & 0xffffu) | ((valU >> 16) & 0xffff0000u);
        }
      }
      else {
        if(is_unsigned) {
          low1 = valU;
          if (low1 > 0xffff) {
              low1 = 0xffff;
              sat = 1;
          }

          high1 = valU >> 32;
          if (high1 > 0xffff) {
              high1 = 0xffff;
              sat = 1;
          }

          valU = low1 | (high1 << 16);
        }
        else {
          lowS1 = valU;
          if (lowS1 != (int16_t)lowS1) {
              lowS1 = (lowS1 >> 31) ^ 0x7fff;
              sat = 1;
          }
          highS1 =  valU >> 32;
          if (highS1 != (int16_t)highS1) {
              highS1 = (highS1 >> 31) ^ 0x7fff;
              sat = 1;
          }
          valU = (uint16_t)lowS1 | (highS1 << 16);
        }
      }

      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t shift = (int8_t)shiftop;
          if (shift >= 32 || shift < -32) {
            tmpres32U = 0;
          } 
          else if (shift == -32) {
            tmpres32U = qlow >> 31;
          } 
          else if (shift < 0) {
            uint64_t big_tmpres32U = ((uint64_t)qlow + (1 << (-1 - shift)));
            tmpres32U = big_tmpres32U >> -shift;
          } 
          else {
            tmpres32U = qlow << shift;
          }
        }
        else {
          int32_t dest;
          int32_t val = (int32_t)qlow;
          int8_t shift = (int8_t)shiftop;

          if ((shift >= 32) || (shift <= -32)) {
            dest = 0;
          } 
          else if (shift < 0) {
            int64_t big_dest = ((int64_t)val + (1 << (-1 - shift)));
            dest = big_dest >> -shift;
          } 
          else {
            dest = val << shift;
          }
          tmpres32U = dest;
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 32 || tmp <= -32 ) { 
            tmpres32U = 0; 
          }
          else if (tmp < 0) { 
            tmpres32U = qlow >> -tmp; 
          }
          else { 
            tmpres32U = qlow << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 32) { 
            tmpres32U = 0; 
          } 
          else if (tmp <= -32) { 
            tmpres32U = qlow >> 31; 
          } 
          else if (tmp < 0) { 
            tmpres32U = qlow >> -tmp; 
          } 
          else { 
            tmpres32U = qlow << tmp; 
          }
        }
      }

      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t shift = (int8_t)shiftop;
          if (shift >= 32 || shift < -32) {
            tmpres32U2 = 0;
          } 
          else if (shift == -32) {
            tmpres32U2 = qhigh >> 31;
          } 
          else if (shift < 0) {
            uint64_t big_tmpres32U2 = ((uint64_t)qhigh + (1 << (-1 - shift)));
            tmpres32U2 = big_tmpres32U2 >> -shift;
          } 
          else {
            tmpres32U2 = qhigh << shift;
          }
        }
        else {
          int32_t dest;
          int32_t val = (int32_t)qhigh;
          int8_t shift = (int8_t)shiftop;

          if ((shift >= 32) || (shift <= -32)) {
            dest = 0;
          } 
          else if (shift < 0) {
            int64_t big_dest = ((int64_t)val + (1 << (-1 - shift)));
            dest = big_dest >> -shift;
          } 
          else {
            dest = val << shift;
          }
          tmpres32U2 = dest;
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 32 || tmp <= -32 ) { 
            tmpres32U2 = 0; 
          }
          else if (tmp < 0) { 
            tmpres32U2 = qhigh >> -tmp; 
          }
          else { 
            tmpres32U2 = qhigh << tmp; 
          }
        }
        else {
          int8_t tmp; 
          tmp = (int8_t)shiftop; 
          if (tmp >= 32) { 
            tmpres32U2 = 0; 
          } 
          else if (tmp <= -32) { 
            tmpres32U2 = qhigh >> 31; 
          } 
          else if (tmp < 0) { 
            tmpres32U2 = qhigh >> -tmp; 
          } 
          else { 
            tmpres32U2 = qhigh << tmp; 
          }
        }
      }

      valU2 = ((uint64_t)tmpres32U2 << 32) | tmpres32U;

      //narrow
      if(!op) {
        if(is_unsigned) {
          low1 = valU2;
          if (low1 & 0x80000000) {
            low1 = 0;
            sat = 1;
          }
          else if (low1 > 0xffff) {
            low1 = 0xffff;
            sat = 1;
          }

          high1 = valU2 >> 32;
          if (high1 & 0x80000000) {
            high1 = 0;
            sat = 1;
          }
          else if (high1 > 0xffff) {
            high1 = 0xffff;
            sat = 1;
          }

          valU2 = (low1 | (high1 << 16));
        }
        else {
          valU2 = (valU2 & 0xffffu) | ((valU2 >> 16) & 0xffff0000u);
        }
      }
      else {
        if(is_unsigned) {
          low1 = valU2;
          if (low1 > 0xffff) {
              low1 = 0xffff;
              sat = 1;
          }

          high1 = valU2 >> 32;
          if (high1 > 0xffff) {
              high1 = 0xffff;
              sat = 1;
          }

          valU2 = low1 | (high1 << 16);
        }
        else {
          lowS1 = valU2;
          if (lowS1 != (int16_t)lowS1) {
              lowS1 = (lowS1 >> 31) ^ 0x7fff;
              sat = 1;
          }
          highS1 = valU2 >> 32;
          if (highS1 != (int16_t)highS1) {
              highS1 = (highS1 >> 31) ^ 0x7fff;
              sat = 1;
          }
          valU2 = (uint16_t)lowS1 | (highS1 << 16);
        }
      }

      break;
    case 3: //64-bit
      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t shift = (uint8_t)shiftop;
          valU = ((uint64_t)high << 32) | low;
          if (shift >= 64 || shift < -64) {
            valU = 0;
          }
          else if (shift == -64) {
            /* Rounding a 1-bit result just preserves that bit.  */
            valU >>= 63;
          }
          else if (shift < 0) {
            valU >>= (-shift - 1);
            if (valU == UNSIGNEDMAX64) {
              /* In this case, it means that the rounding constant is 1,
               * and the addition would overflow. Return the actual
               * result directly.  */
              valU = 0x8000000000000000ULL;
            } 
            else {
              valU++;
              valU >>= 1;
            }
          } 
          else {
            valU <<= shift;
          }
        }
        else {
          int8_t shift = (int8_t)shiftop;
          valU = ((uint64_t)high << 32) | low;
          int64_t val = valU;

          if ((shift >= 64) || (shift <= -64)) {
            val = 0;
          } 
          else if (shift < 0) {
            val >>= (-shift - 1);
            if (val == INT64_MAX) {
              /* In this case, it means that the rounding constant is 1,
               * and the addition would overflow. Return the actual
               * result directly.  */
              val = 0x4000000000000000LL;
            } 
            else {
              val++;
              val >>= 1;
            }
          } 
          else {
            val <<= shift;
          }
          valU = val;
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t shift = (int8_t)shiftop;
          valU = ((uint64_t)high << 32) | low;

          if (shift >= 64 || shift <= -64) {
            valU = 0;
          } 
          else if (shift < 0) {
            valU >>= -shift;
          } 
          else {
            valU <<= shift;
          }
        }
        else {
          int8_t shift = (int8_t)shiftop;

          valU = ((uint64_t)high << 32) | low;
          int64_t val = valU;
          if (shift >= 64) {
            val = 0;
          } 
          else if (shift <= -64) {
            val >>= 63;
          } 
          else if (shift < 0) {
            val >>= -shift;
          } 
          else {
            val <<= shift;
          }
          valU = val;
        }
      }

      //narrow
      if(!op) {
        if(is_unsigned) {
          if (valU & 0x8000000000000000ull) {
            sat = 1;
            valU = 0;
          }
          if (valU > 0xffffffffu) {
            sat = 1;
            valU = 0xffffffffu;
          }
        }
        else {
          valU = valU & 0xFFFFFFFF;
        }
      }
      else {
        if(is_unsigned) {
          if (valU > 0xffffffffu) {
            sat = 1;
            valU = 0xffffffffu;
          }
        }
        else {
          if ((int64_t)valU != (int32_t)valU) {
            sat = 1;
            valU = ((int64_t)valU >> 63) ^ 0x7fffffff;
          }
        }
      }

      //next 64-bits
      if(round) { //VQRSHRN
        if(input_unsigned) {
          int8_t shift = (uint8_t)shiftop;
          valU2 = ((uint64_t)qhigh << 32) | qlow;

          if (shift >= 64 || shift < -64) {
            valU2 = 0;
          }
          else if (shift == -64) {
            /* Rounding a 1-bit result just preserves that bit.  */
            valU2 >>= 63;
          }
          else if (shift < 0) {
            valU2 >>= (-shift - 1);
            if (valU2 == UNSIGNEDMAX64) {
              /* In this case, it means that the rounding constant is 1,
               * and the addition would overflow. Return the actual
               * result directly.  */
              valU2 = 0x8000000000000000ULL;
            } 
            else {
              valU2++;
              valU2 >>= 1;
            }
          } 
          else {
            valU2 <<= shift;
          }
        }
        else {
          int8_t shift = (int8_t)shiftop;

          valU2 = ((uint64_t)qhigh << 32) | qlow;
          int64_t val = valU2;

          if ((shift >= 64) || (shift <= -64)) {
            val = 0;
          } 
          else if (shift < 0) {
            val >>= (-shift - 1);
            if (val == INT64_MAX) {
              /* In this case, it means that the rounding constant is 1,
               * and the addition would overflow. Return the actual
               * result directly.  */
              val = 0x4000000000000000LL;
            } 
            else {
              val++;
              val >>= 1;
            }
          } 
          else {
            val <<= shift;
          }
          valU2 = val;
        }
      }
      else { //VQSHRN
        if(input_unsigned) {
          int8_t shift = (int8_t)shiftop;
          valU2 = ((uint64_t)qhigh << 32) | qlow;

          if (shift >= 64 || shift <= -64) {
            valU2 = 0;
          } 
          else if (shift < 0) {
            valU2 >>= -shift;
          } 
          else {
            valU2 <<= shift;
          }
        }
        else {
          int8_t shift = (int8_t)shiftop;
          valU2 = ((uint64_t)qhigh << 32) | qlow;
          int64_t val = valU2;

          if (shift >= 64) {
            val = 0;
          } 
          else if (shift <= -64) {
            val >>= 63;
          } 
          else if (shift < 0) {
            val >>= -shift;
          } 
          else {
            val <<= shift;
          }
          valU2 = val;
        }
      }

      //narrow
      if(!op) {
        if(is_unsigned) {
          if (valU2 & 0x8000000000000000ull) {
            sat = 1;
            valU2 = 0;
          }
          if (valU2 > 0xffffffffu) {
            sat = 1;
            valU2 = 0xffffffffu;
          }
        }
        else {
          valU2 = valU2 & 0xFFFFFFFF;
        }
      }
      else {
        if(is_unsigned) {
          if (valU2 > 0xffffffffu) {
            sat = 1;
            valU2 = 0xffffffffu;
          }
        }
        else {
          if ((int64_t)valU2 != (int32_t)valU2) {
            sat = 1;
            valU2 = ((int64_t)valU2 >> 63) ^ 0x7fffffff;
          }
        }
      }
      break;
  }

  prog->st32( arg1 + 16, valU & 0xFFFFFFFF );
  prog->st32( arg1 + 20, valU2 & 0xFFFFFFFF );
  prog->st32( arg1 + 32, sat);

  return 0;
}

uint32_t do_vqmovn(ProgramBase *prog, uint32_t arg1) 
{
  uint32_t low   = prog->ldu32( arg1+4 ) ;
  uint32_t high  = prog->ldu32( arg1+8 ) ;
  uint32_t imm   = prog->ldu32( arg1+12 ) ;

  uint32_t qlow  = prog->ldu32( arg1+36 ) ;
  uint32_t qhigh = prog->ldu32( arg1+40 ) ;

  uint8_t size = (imm & 0x3);
  uint8_t round = (imm >> 2) & 0x1;
  uint8_t op = (imm>>3) & 0x1;

  uint32_t valU, valU2;

  uint8_t sat = 0;

  uint32_t high1,low1;
  int32_t highS1,lowS1;

  uint64_t x = ((uint64_t)high << 32) | low;

  switch(size) {
    case 0: //16-bits
      if(op) {
        if(round) { 
          uint16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = x; 
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d;
          }

          s = x >> 16; 
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 8;
          }

          s = x >> 32; 
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 16;
          }
      
          s = x >> 48; 
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 24;
          }

          valU = res;
        }
        else {
          valU = (x & 0xffu) | ((x >> 8) & 0xff00u) | ((x >> 16) & 0xff0000u)
                 | ((x >> 24) & 0xff000000u);
        }
      }
      else {
        if(round) { 
          uint16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = x;
          if (s > 0xff) {
            d = 0xff;
            sat = 1;
          } 
          else {
            d = s;
          } 
          res |= (uint32_t)d;

          s = x >> 16;
          if (s > 0xff) {
            d = 0xff;
            sat = 1;
          } 
          else {
            d = s;
          } 
          res |= (uint32_t)d << 8;

          s = x >> 32;
          if (s > 0xff) {
            d = 0xff;
            sat = 1;
          } 
          else {
            d = s;
          } 
          res |= (uint32_t)d << 16;

          s = x >> 48;
          if (s > 0xff) {
            d = 0xff;
            sat = 1;
          } 
          else {
            d = s;
          } 
          res |= (uint32_t)d << 24;

          valU = res;
        }
        else {
          int16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = x;
          if (s != (int8_t)s) {
            d = (s >> 15) ^ 0x7f;
            sat = 1;
          } 
          else {
            d = s;
          }
          res |= (uint32_t)d;

          s = x >> 16;
          if (s != (int8_t)s) {
            d = (s >> 15) ^ 0x7f;
            sat = 1;
          } 
          else {
            d = s;
          }
          res |= (uint32_t)d << 8;

          s = x >> 32;
          if (s != (int8_t)s) {
            d = (s >> 15) ^ 0x7f;
            sat = 1;
          } 
          else {
            d = s;
          }
          res |= (uint32_t)d << 16;

          s = x >> 48;
          if (s != (int8_t)s) {
            d = (s >> 15) ^ 0x7f;
            sat = 1;
          } 
          else {
            d = s;
          }
          res |= (uint32_t)d << 24;

          valU = res;
        }
      }

      x = ((uint64_t)qhigh << 32) | qlow;
      if(op) {
        if(round) { 
          uint16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = x; 
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d;
          }

          s = x >> 16; 
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 8;
          }

          s = x >> 32; 
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 16;
          }
      
          s = x >> 48; 
          if (s & 0x8000) { 
            sat = 1;
          } 
          else { 
            if (s > 0xff) { 
              d = 0xff; 
              sat = 1;
            } 
            else  { 
              d = s; 
            } 

            res |= (uint32_t)d << 24;
          }

          valU2 = res;
        }
        else {
          valU2 = (x & 0xffu) | ((x >> 8) & 0xff00u) | ((x >> 16) & 0xff0000u)
                 | ((x >> 24) & 0xff000000u);
        }
      }
      else {
        if(round) { 
          uint16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = x;
          if (s > 0xff) {
            d = 0xff;
            sat = 1;
          } 
          else {
            d = s;
          } 
          res |= (uint32_t)d;

          s = x >> 16;
          if (s > 0xff) {
            d = 0xff;
            sat = 1;
          } 
          else {
            d = s;
          } 
          res |= (uint32_t)d << 8;

          s = x >> 32;
          if (s > 0xff) {
            d = 0xff;
            sat = 1;
          } 
          else {
            d = s;
          } 
          res |= (uint32_t)d << 16;

          s = x >> 48;
          if (s > 0xff) {
            d = 0xff;
            sat = 1;
          } 
          else {
            d = s;
          } 
          res |= (uint32_t)d << 24;

          valU2 = res;
        }
        else {
          int16_t s;
          uint8_t d;
          uint32_t res = 0;

          s = x;
          if (s != (int8_t)s) {
            d = (s >> 15) ^ 0x7f;
            sat = 1;
          } 
          else {
            d = s;
          }
          res |= (uint32_t)d;

          s = x >> 16;
          if (s != (int8_t)s) {
            d = (s >> 15) ^ 0x7f;
            sat = 1;
          } 
          else {
            d = s;
          }
          res |= (uint32_t)d << 8;

          s = x >> 32;
          if (s != (int8_t)s) {
            d = (s >> 15) ^ 0x7f;
            sat = 1;
          } 
          else {
            d = s;
          }
          res |= (uint32_t)d << 16;

          s = x >> 48;
          if (s != (int8_t)s) {
            d = (s >> 15) ^ 0x7f;
            sat = 1;
          } 
          else {
            d = s;
          }
          res |= (uint32_t)d << 24;

          valU2 = res;
        }
      }
      break;
    case 1: //32-bits
      if(op) {
        if(round) {
          low1 = x;
          if (low1 & 0x80000000) {
            low1 = 0;
            sat = 1;
          } 
          else if (low1 > 0xffff) {
            low1 = 0xffff;
            sat = 1;
          }

          high1 = x >> 32;
          if (high1 & 0x80000000) {
            high1 = 0;
            sat = 1;
          } 
          else if (high1 > 0xffff) {
            high1 = 0xffff;
            sat = 1;
          }

          valU = low1 | (high1 << 16);
        }
        else {
          valU = (x & 0xffffu) | ((x >> 16) & 0xffff0000u);
        }
      }
      else {
        if(round) {
          low1 = x;
          if (low1 > 0xffff) {
            low1 = 0xffff;
            sat = 1;
          }

          high1 = x >> 32;
          if (high1 > 0xffff) {
            high1 = 0xffff;
            sat = 1;
          }

          valU = low1 | (high1 << 16);
        }
        else {
          lowS1 = x;
          if (lowS1 != (int16_t)lowS1) {
              lowS1 = (lowS1 >> 31) ^ 0x7fff;
              sat = 1;
          }

          highS1 = x >> 32;
          if (highS1 != (int16_t)highS1) {
              highS1 = (highS1 >> 31) ^ 0x7fff;
              sat = 1;
          }

          valU = (uint16_t)lowS1 | (highS1 << 16);
        }
      }

      x = ((uint64_t)qhigh << 32) | qlow;
      if(op) {
        if(round) {
          low1 = x;
          if (low1 & 0x80000000) {
            low1 = 0;
            sat = 1;
          } 
          else if (low1 > 0xffff) {
            low1 = 0xffff;
            sat = 1;
          }

          high1 = x >> 32;
          if (high1 & 0x80000000) {
            high1 = 0;
            sat = 1;
          } 
          else if (high1 > 0xffff) {
            high1 = 0xffff;
            sat = 1;
          }

          valU2 = low1 | (high1 << 16);
        }
        else {
          valU2 = (x & 0xffffu) | ((x >> 16) & 0xffff0000u);
        }
      }
      else {
        if(round) {
          low1 = x;
          if (low1 > 0xffff) {
            low1 = 0xffff;
            sat = 1;
          }

          high1 = x >> 32;
          if (high1 > 0xffff) {
            high1 = 0xffff;
            sat = 1;
          }

          valU2 = low1 | (high1 << 16);
        }
        else {
          lowS1 = x;
          if (lowS1 != (int16_t)lowS1) {
              lowS1 = (lowS1 >> 31) ^ 0x7fff;
              sat = 1;
          }

          highS1 = x >> 32;
          if (highS1 != (int16_t)highS1) {
              highS1 = (highS1 >> 31) ^ 0x7fff;
              sat = 1;
          }

          valU2 = (uint16_t)lowS1 | (highS1 << 16);
        }
      }
      break;
    case 2: //64-bits
      if(op) {
        if(round) {
          if (x & 0x8000000000000000ull) {
            sat = 1;
            valU = 0;
          }
          if (x > 0xffffffffu) {
            sat = 1;
            valU = 0xffffffffu;
          }
        }
        else {
          valU = x & 0xFFFFFFFF;
        }
      }
      else {
        if(round) {
          if (x > 0xffffffffu) {
            sat = 1;
            valU = 0xffffffffu;
          }
        }
        else {
          if ((int64_t)x != (int32_t)x) {
              sat = 1;
              valU = ((int64_t)x >> 63) ^ 0x7fffffff;
          }
        }
      }

      x = ((uint64_t)qhigh << 32) | qlow;
      if(op) {
        if(round) {
          if (x & 0x8000000000000000ull) {
            sat = 1;
            valU2 = 0;
          }
          if (x > 0xffffffffu) {
            sat = 1;
            valU2 = 0xffffffffu;
          }
        }
        else {
          valU2 = x & 0xFFFFFFFF;
        }
      }
      else {
        if(round) {
          if (x > 0xffffffffu) {
            sat = 1;
            valU2 = 0xffffffffu;
          }
        }
        else {
          if ((int64_t)x != (int32_t)x) {
              sat = 1;
              valU2 = ((int64_t)x >> 63) ^ 0x7fffffff;
          }
        }
      }
      break;
  }
  prog->st32( arg1 + 16, valU );
  prog->st32( arg1 + 20, valU2 );
  prog->st32( arg1 + 32, sat);

  return 0;
}

uint32_t do_vcvtb_htos(ProgramBase *prog, uint32_t arg1) {
  uint32_t val = prog->ldu32(arg1+4);
  uint32_t xs;
  uint32_t xe;
  uint32_t xm;

  int32_t xes;

  int e;

  uint16_t h = val & 0xFFFF;
  uint16_t hs;
  uint16_t he;
  uint16_t hm;

        if((h & 0x7FFFu) == 0) { // Signed zero
          val = ((uint32_t) h) << 16;
        } else {
            hs = h & 0x8000u;
            he = h & 0x7C00u;
            hm = h & 0x03FFu;
            if(he == 0) { // Denormalized will convert to normalized
              e = -1;
              do {
                e++;
                hm <<= 1;
              } while((hm & 0x0400u) == 0);
              xs = ((uint32_t) hs) << 16;
              xes = ((int32_t) (he >> 10)) - 15 + 127 - e;
              xe = (uint32_t) (xes << 23);
              xm = ((uint32_t) (hm & 0x03FFu)) << 13;
              val = (xs | xe | xm);
            } else if(he == 0x7C00u) { // Inf or NaN
                if(hm == 0) {
                  val = (((uint32_t) hs) << 16) | ((uint32_t) 0x7F800000u); // Signed Inf
                } else {
                    val = (uint32_t) 0xFFC00000u; // NaN, only 1st mantissa bit set
                }
            } else { // Normalized number
                xs = ((uint32_t) hs) << 16;
                xes = ((int32_t) (he >> 10)) - 15 + 127;
                xe = (uint32_t) (xes << 23);
                xm = ((uint32_t) hm) << 13;
                val = (xs | xe | xm);
            }
        }

  prog->st32(arg1+16,val);

  return 0;
}

uint32_t do_vcvtt_htos(ProgramBase *prog, uint32_t arg1) {
  uint32_t val = prog->ldu32(arg1+4);
        uint32_t xs;
        uint32_t xe;
        uint32_t xm;

        int32_t xes;

        int e;

        uint16_t h = (val >> 16) & 0xFFFF;
        uint16_t hs;
        uint16_t he;
        uint16_t hm;

        if((h & 0x7FFFu) == 0) { // Signed zero
          val = ((uint32_t) h) << 16;
        } else {
            hs = h & 0x8000u;
            he = h & 0x7C00u;
            hm = h & 0x03FFu;
            if(he == 0) { // Denormal will convert to normalized
              e = -1;
              do {
                e++;
                hm <<= 1;
              } while((hm & 0x0400u) == 0);
              xs = ((uint32_t) hs) << 16;
              xes = ((int32_t) (he >> 10)) - 15 + 127 - e;
              xe = (uint32_t) (xes << 23);
              xm = ((uint32_t) (hm & 0x03FFu)) << 13;
              val = (xs | xe | xm);
            } else if(he == 0x7C00u) { // Inf or NaN
                if(hm == 0) {
                  val = (((uint32_t) hs) << 16) | ((uint32_t) 0x7F800000u); // Signed Inf
                } else {
                    val = (uint32_t) 0xFFC00000u; // NaN, only 1st mantissa bit set
                }
            } else { // Normalized number
                xs = ((uint32_t) hs) << 16;
                xes = ((int32_t) (he >> 10)) - 15 + 127;
                xe = (uint32_t) (xes << 23);
                xm = ((uint32_t) hm) << 13;
                val = (xs | xe | xm);
            }
        }

        prog->st32(arg1+16,val);

  return 0;
}

uint32_t do_vcvtb_stoh(ProgramBase *prog, uint32_t arg1) {
        uint32_t val = prog->ldu32(arg1+4);
  uint32_t des = prog->ldu32(arg1+8);
        uint32_t xs;
        uint32_t xe;
        uint32_t xm;

        uint16_t h;
        uint16_t hs;
        uint16_t he;
        uint16_t hm;

  int hes;

        if((val & 0x7FFFFFFFu) == 0) { // Signed zero
            h = (uint16_t) (val >> 16);
        } else {
            xs = val & 0x80000000u;
            xe = val & 0x7F800000u;
            xm = val & 0x007FFFFFu;
            if(xe == 0) { // Denormal will underflow, return a signed zero
              h = (uint16_t) (xs >> 16);
            } else if(xe == 0x7F800000u) { // Inf or NaN
                if(xm == 0) {
                  h = (uint16_t) ((xs >> 16) | 0x7C00u); // Signed Inf
                } else {
                    h = (uint16_t) 0xFE00u; // NaN, only 1st mantissa bit set
                }
            } else { // Normalized number
                hs = (uint16_t) (xs >> 16);
                hes = ((int)(xe >> 23)) - 127 + 15;
                if(hes >= 0x1F) { // Overflow
                  h = (uint16_t) ((xs >> 16) | 0x7C00u); // Signed Inf
                } else if(hes <= 0) { // Underflow
                    if((14 - hes) > 24) { // Mantissa shifted all the way off & no rounding possibility
                      hm = (uint16_t) 0u;
                    } else {
                        xm |= 0x00800000u;
                        hm = (uint16_t) (xm >> (14 - hes));
                        if((xm >> (13 - hes)) & 0x00000001u) { // Check for rounding
                          hm += (uint16_t) 1u;
                      }
        }
                    h = (hs | hm);
                } else {
                    he = (uint16_t) (hes << 10);
                    hm = (uint16_t) (xm >> 13);
                    if(xm & 0x00001000u) { // Check for rounding
                      h = (hs | he | hm) + (uint16_t) 1u;
                    } else {
                        h = (hs | he | hm);
                    }
    }
            }
        }

  des = ((des & 0xFFFF0000) | h);
  prog->st32(arg1+16,des);

  return 0;
}

uint32_t do_vcvtt_stoh(ProgramBase *prog, uint32_t arg1) {
        uint32_t val = prog->ldu32(arg1+4);
  uint32_t des = prog->ldu32(arg1+8);
        uint32_t xs;
        uint32_t xe;
        uint32_t xm;

        uint16_t h;
        uint16_t hs;
        uint16_t he;
        uint16_t hm;

  int hes;

        if((val & 0x7FFFFFFFu) == 0) { // Signed zero
          h = (uint16_t) (val >> 16);
        } else {
            xs = val & 0x80000000u;
            xe = val & 0x7F800000u;
            xm = val & 0x007FFFFFu;
            if(xe == 0) { // Denormal will underflow, return a signed zero
              h = (uint16_t) (xs >> 16);
            } else if(xe == 0x7F800000u) { // Inf or NaN
                if(xm == 0) {
                  h = (uint16_t) ((xs >> 16) | 0x7C00u); // Signed Inf
                } else {
                    h = (uint16_t) 0xFE00u; // NaN, only 1st mantissa bit set
                }
            } else { // Normalized number
                hs = (uint16_t) (xs >> 16);
                hes = ((int)(xe >> 23)) - 127 + 15;
                if(hes >= 0x1F) { // Overflow
                  h = (uint16_t) ((xs >> 16) | 0x7C00u); // Signed Inf
                } else if(hes <= 0) { // Underflow
                    if((14 - hes) > 24) { // Mantissa shifted all the way off & no rounding possibility
                      hm = (uint16_t) 0u;
                    } else {
                        xm |= 0x00800000u;
                        hm = (uint16_t) (xm >> (14 - hes));
                        if((xm >> (13 - hes)) & 0x00000001u) { // Check for rounding
                          hm += (uint16_t) 1u;
      }
                    }
                    h = (hs | hm);
                } else {
                    he = (uint16_t) (hes << 10);
                    hm = (uint16_t) (xm >> 13);
                    if(xm & 0x00001000u) { // Check for rounding
                      h = (hs | he | hm) + (uint16_t) 1u;
                    } else {
                      h = (hs | he | hm);
         }
                }
            }
        }

        des = (((h << 16) & 0xFFFFFFFF) | (des & 0x0000FFFF));
        prog->st32(arg1+16,des);

        return 0;
}
