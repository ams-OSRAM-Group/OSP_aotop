// manc.cpp - Manchester decoder for OSP (convert bit flip time stamps to bits)
/*****************************************************************************
 * Copyright 2026 by ams OSRAM AG                                            *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************/


// Assumptions
// - default line level is high
// - preamble is 0xA
// - L-to-H transition means 1
// - length of bit train is max 12 bytes
// - captured by ESP32S3s RMT block at 19 200 000 Hz


#include <Arduino.h>           // eg Serial, micros()
#include "manc.h"              // self


// The RMT block gives a notification on each level change.
// The state is tracked at those change moments.
// States are named with 3 characters [0,1][B,M][0,1]:
// - When the level was 0, but RMT captured a transition to 1, the state is 0X1
// - When the level was 1, but RMT captured a transition to 0, the state is 1X0.
// - The level changes can happen at the beginning of a bit xBx, or midway a bit xMx.

// Transitions of the state machine
//       +---+   4   +---+   8   +---+   4   +---+
// *---->|   |------>|   |------>|   |------>|   |
//       |1B0|       |0M1|       |1M0|       |0B1|
//       |   |<------|   |<------|   |<------|   |
//       +---+   4   +---+   8   +---+   4   +---+
// From the Begin of a bit, only a short (4 ticks) transition is possible, going to Mid bit.
// From the Mid of a bit, two transitions are possible: a short (4 ticks) to Begin bit, and a long (8 ticks) to Mid bit.

// Note that the OSP frequency is 2.4MHz and that this decoder assumes the RMT capture block runs at 19.2MHz. 
// This results in bit times to be 8 ticks and half bits to be 4 ticks.
// That explains the magic constants in this code ("long" and "short").

// - Example 1
//
//   position     B   M   B   M   B   M   B   M   B   M   B   M   B   M   B   M   B  (Begin or Mid bit)
//   rmtsymbols   |<<<<< >>>>>|<<<<< >>>>>|<<<<< >>>>>|<<< >>>|<<< >>>|<<<<< >>>>>|
//   duration     |<4>|<<<8>>>|<<<8>>>|<4>|<4>|<<<8>>>|<4>|<4>|<4>|<4>|<<<8>>>|<0>|  (4=short,8=long)
//   hi-level  ---+   +---+---+       +---+   +---+---+   +---+   +---+       +---+---
//                |   |       |       |   |   |       |   |   |   |   |       |
//   lo-level     +---+       +---+---+   +---+       +---+   +---+   +---+---+
//   state       1B0 0M1     1M0     0M1             1M0 0B1 1M0 0B1 1M0     0M1 
//   output       |<<<1>>>|<<<0>>>|<<<1>>>|<<<1>>>|<<<0>>>|<<<0>>>|<<<0>>>|<<<1>>>|
//   hex          |               B               |               1               |

// - Example 2
//
//   position     B   M   B   M   B   M   B   M   B   M   B   M   B   M   B   M   B   M  (Begin or Mid bit)
//   rmtsymbols   |<<<<< >>>>>|<<<<<<< >>>>>>>|<<<<<<< >>>>>>>|<<<<<<< >>>>>>>|<<< >>>|
//   duration     |<4>|<<<8>>>|<<<8>>>|<<<8>>>|<<<8>>>|<<<8>>>|<<<8>>>|<<<8>>>|<4>|<0>|  (4=short,8=long)
//   hi-level  ---+   +---+---+       +---+---+       +---+---+       +---+---+   +---+---
//                |   |       |       |       |       |       |       |       |   |
//   lo-level     +---+       +---+---+       +---+---+       +---+---+       +---+
//   state       1B0 0M1     1M0     0M1     1M0     0M1     1M0     0M1     1M0 0B1      
//   output       |<<<1>>>|<<<0>>>|<<<1>>>|<<<0>>>|<<<1>>>|<<<0>>>|<<<1>>>|<<<0>>>|
//   hex          |               A               |               A               |


// The state of the Manchester decoder.
typedef enum manc_state_e {
  manc_state_0B1,
  manc_state_0M1,
  manc_state_1B0,
  manc_state_1M0,
} manc_state_t;


// Convert a state to a string
static const char* manc_state_strings[]= {"0B1","0M1","1B0","1M0"};


// Convert a decoder result to a string
static const char* manc_result_strings[manc_result_assert+1]= {"ok", "wronglevel", "toolong", "wrongduration", "toomanybytes", "no8fold", "assert"};
const char* manc_result_str( manc_result_t result ) {
  return manc_result_strings[result];
}


// Use fast or slower but more save code. Gain of fast is about 20% (test case 3 from 6481ns to 5082ns)
#define FAST // define for fast, comment out for more safety
#ifdef FAST  
  #define MANC_ASSERT(p) do { } while(0)
  // #warning manc.cpp is configured for FAST, not for SAVE
#else
  #define MANC_ASSERT(p) do { if( !(p) ) return manc_result_assert; } while(0)
#endif


// This type is needed only when compiling for fast code
// We use this type to cast one symbol_word to two pulse_halfword's (which is undefined behavior).
typedef struct { uint16_t duration:15; uint16_t level:1; } manc_pulse_halfword_t;


// Decode a symbols array to a bytes array
manc_result_t manc_decode(const rmt_symbol_word_t *symbols, uint8_t *bytes, int *bytecount ) {
  // Parameter checks
  MANC_ASSERT( symbols!=0 || bytes!=0 || bytecount!=0 || *bytecount>0 );

  // Record the bytes[] buffer size; it's being overwritten: upon return bytecount is the decoded amount of bytes.
  int remaining= *bytecount;

  // Prevent warning when debug prints are commented out
  (void)manc_state_strings;

  // (1) initialize state machine 
  // A Manchester signal is (sometimes) ambiguous, for example take three low pulses: HHHLHLHLHHH.
  // This could mean HH(HLHLHL)HHH=000, or HHH(LHLHLH)HH=111.
  // In practice, Manchester signals start with a preamble, fixing the first bit(s).
  // This routine assumes a default high level, and that first (preamble) bit is a one.
  // The preamble assumption is enforced by the following starting state.
  manc_state_t state= manc_state_1B0;
  // If the capturing was mid-telegram, the preamble assumption would be invalid.
  if( symbols[0].level0!=0 ) return manc_result_wronglevel; 
  // Recall that symbols[i].level0 and symbols[i].level1 are bits, so restricted in value to 0 or 1 ("excluded middle").
  // By virtue of the capturing process, for all i, symbols[i].level0!=symbols[i].level1 and symbols[i].level1!=symbols[i+1].level0.
  // With the preamble assumption this implies that for all i symbols[i].level0==0 and symbols[i].level1==1.
  
  // (2) init for emitting bits (in bytes array)
  *bytecount= 0; // output argument: number of bytes decoded is 0
  bytes--; // pointer 1 back, because we increment for each byte to emit (undefined behavior in C++: decrementing outside array bounds)
  int bitmask= 0x01; // The mask for the previously bit written: 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01

  // (3) loop over all pulses
  #ifdef FAST
    manc_pulse_halfword_t * pulses= (manc_pulse_halfword_t *)symbols; // Map an array of words to an array of half-words. Undefined behavior.
  #endif
  for(int pulse=0,cont=1; cont ; pulse++) {

    // (3a) get the (level and) duration of the pulse
    #ifdef FAST
      // lvl not checked in FAST mode - only once in step(1)
      int dur=pulses[pulse].duration;
    #else 
      int lvl;
      int dur;
      if( pulse%2==0 ) {
        lvl= symbols[pulse/2].level0;
        dur= symbols[pulse/2].duration0;
      } else {
        lvl= symbols[pulse/2].level1;
        dur= symbols[pulse/2].duration1;
      }
    #endif
    // Serial.printf("%s %d (%d,%d)\n",manc_state_strings[state],pulse,lvl,dur);

    // (3b) move state machine to next state
    #define isatend(d) (           (d)==0 )
    #define isshort(d) ( 3<=(d) && (d)<=5 ) // 4 ticks -1/+1
    #define islong(d)  ( 7<=(d) && (d)<=10) // 8 ticks -1/+2
    switch( state ) { 
      // A switch() is faster than cascaded if()
      // In each case there is an if-elseif, with most likely case first
      case manc_state_0B1:
        MANC_ASSERT( lvl==1 );
        if( isshort(dur) ) state=manc_state_1M0;
        else if( isatend(dur) ) { cont=0; continue; } // at endmarker, and not at mid-bit (continue io break because we need 'break _for_' not 'break _switch_').
        else if( islong(dur) ) return manc_result_expectshort;
        else return manc_result_wrongduration;
      break;
      case manc_state_0M1:
        MANC_ASSERT( lvl==1 );
        if( isshort(dur) ) state=manc_state_1B0;
        else if( islong(dur) ) state=manc_state_1M0;
        else if( isatend(dur) ) { state=manc_state_1B0; cont=0; dur=4; } // use 4 ticks from default hi level, see example 1
        else return manc_result_wrongduration;
      break;
      case manc_state_1B0:
        MANC_ASSERT( lvl==0 );
        if( isshort(dur) ) state=manc_state_0M1;
        else if( islong(dur) ) return manc_result_expectshort;
        // can not be atend when lvl==0
        else return manc_result_wrongduration;
      break;
      case manc_state_1M0:
        MANC_ASSERT(state==manc_state_1M0 );
        MANC_ASSERT( lvl==0 );
        if( isshort(dur) ) state=manc_state_0B1;
        else if( islong(dur) ) state=manc_state_0M1;
        // can not be atend when lvl==0
        else return manc_result_wrongduration;
      break;
    }

    // (3c) emit the decoded bit
    if( state==manc_state_0M1 || state==manc_state_1M0 ) { 
      // Do we need to move to the next byte?
      if( bitmask==0x01 ) { 
        if( --remaining < 0 ) return manc_result_toomanybytes;
        (*bytecount)++; bytes++; // claim next byte
        *bytes=0; bitmask=0x80;  // init claimed byte
      } else { 
        bitmask>>=1; // move to next bit within current byte
      }
      // Emit bit
      if( state==manc_state_0M1 ) *bytes |= bitmask; else { /* no need to or-in a 0 */ }
    }
  }

  // (4) check if the number of captured bits is an 8-fold
  //Serial.printf("bitmask %d\n",bitmask);
  if( bitmask!=0x01 ) return manc_result_no8fold;

  return manc_result_ok;
}


//=== TEST code =============================================================


// encodes B1 
void manc_test_set1(rmt_symbol_word_t * symbols) {
  symbols[ 0].level0=0; symbols[ 0].level1=1; symbols[ 0].duration0=4; symbols[ 0].duration1=8;
  symbols[ 1].level0=0; symbols[ 1].level1=1; symbols[ 1].duration0=8; symbols[ 1].duration1=4;
  symbols[ 2].level0=0; symbols[ 2].level1=1; symbols[ 2].duration0=4; symbols[ 2].duration1=8;
  symbols[ 3].level0=0; symbols[ 3].level1=1; symbols[ 3].duration0=4; symbols[ 3].duration1=4;
  symbols[ 4].level0=0; symbols[ 4].level1=1; symbols[ 4].duration0=4; symbols[ 4].duration1=4;
  symbols[ 5].level0=0; symbols[ 5].level1=1; symbols[ 5].duration0=8; symbols[ 5].duration1=0;
}


// encodes AA 
void manc_test_set2(rmt_symbol_word_t * symbols) {
  symbols[ 0].level0=0; symbols[ 0].level1=1; symbols[ 0].duration0=4; symbols[ 0].duration1=8;
  symbols[ 1].level0=0; symbols[ 1].level1=1; symbols[ 1].duration0=8; symbols[ 1].duration1=8;
  symbols[ 2].level0=0; symbols[ 2].level1=1; symbols[ 2].duration0=8; symbols[ 2].duration1=8;
  symbols[ 3].level0=0; symbols[ 3].level1=1; symbols[ 3].duration0=8; symbols[ 3].duration1=8;
  symbols[ 4].level0=0; symbols[ 4].level1=1; symbols[ 4].duration0=4; symbols[ 4].duration1=0;
}


// encodes A1 5C 40 2B
void manc_test_set3(rmt_symbol_word_t * symbols) {
  symbols[ 0].level0=0; symbols[ 0].level1=1; symbols[ 0].duration0=3; symbols[ 0].duration1=9;
  symbols[ 1].level0=0; symbols[ 1].level1=1; symbols[ 1].duration0=8; symbols[ 1].duration1=9;
  symbols[ 2].level0=0; symbols[ 2].level1=1; symbols[ 2].duration0=3; symbols[ 2].duration1=5;
  symbols[ 3].level0=0; symbols[ 3].level1=1; symbols[ 3].duration0=4; symbols[ 3].duration1=4;
  symbols[ 4].level0=0; symbols[ 4].level1=1; symbols[ 4].duration0=4; symbols[ 4].duration1=5;
  symbols[ 5].level0=0; symbols[ 5].level1=1; symbols[ 5].duration0=7; symbols[ 5].duration1=9;
  symbols[ 6].level0=0; symbols[ 6].level1=1; symbols[ 6].duration0=8; symbols[ 6].duration1=9;
  symbols[ 7].level0=0; symbols[ 7].level1=1; symbols[ 7].duration0=7; symbols[ 7].duration1=5;
  symbols[ 8].level0=0; symbols[ 8].level1=1; symbols[ 8].duration0=4; symbols[ 8].duration1=4;
  symbols[ 9].level0=0; symbols[ 9].level1=1; symbols[ 9].duration0=4; symbols[ 9].duration1=9;
  symbols[10].level0=0; symbols[10].level1=1; symbols[10].duration0=4; symbols[10].duration1=4;
  symbols[11].level0=0; symbols[11].level1=1; symbols[11].duration0=4; symbols[11].duration1=5;
  symbols[12].level0=0; symbols[12].level1=1; symbols[12].duration0=7; symbols[12].duration1=9;
  symbols[13].level0=0; symbols[13].level1=1; symbols[13].duration0=4; symbols[13].duration1=4;
  symbols[14].level0=0; symbols[14].level1=1; symbols[14].duration0=4; symbols[14].duration1=5;
  symbols[15].level0=0; symbols[15].level1=1; symbols[15].duration0=3; symbols[15].duration1=5;
  symbols[16].level0=0; symbols[16].level1=1; symbols[16].duration0=4; symbols[16].duration1=4;
  symbols[17].level0=0; symbols[17].level1=1; symbols[17].duration0=4; symbols[17].duration1=5;
  symbols[18].level0=0; symbols[18].level1=1; symbols[18].duration0=3; symbols[18].duration1=5;
  symbols[19].level0=0; symbols[19].level1=1; symbols[19].duration0=3; symbols[19].duration1=5;
  symbols[20].level0=0; symbols[20].level1=1; symbols[20].duration0=8; symbols[20].duration1=9;
  symbols[21].level0=0; symbols[21].level1=1; symbols[21].duration0=8; symbols[21].duration1=8;
  symbols[22].level0=0; symbols[22].level1=1; symbols[22].duration0=8; symbols[22].duration1=5;
  symbols[23].level0=0; symbols[23].level1=1; symbols[23].duration0=3; symbols[23].duration1=0;
}


void manc_test() {
  rmt_symbol_word_t symbols[96];
  uint8_t bytes[12];
  manc_result_t result;
  int bytecount;

  manc_test_set1(symbols);
  bytecount= sizeof bytes; 
  result= manc_decode(symbols,bytes,&bytecount);
  Serial.printf("bytes (%d)",bytecount); for( int i=0; i<bytecount; i++ ) Serial.printf(" %02X",bytes[i]); Serial.printf(" %d/%s\n",result,manc_result_strings[result]);

  manc_test_set2(symbols);
  bytecount= sizeof bytes; 
  result= manc_decode(symbols,bytes,&bytecount);
  Serial.printf("bytes (%d)",bytecount); for( int i=0; i<bytecount; i++ ) Serial.printf(" %02X",bytes[i]); Serial.printf(" %d/%s\n",result,manc_result_strings[result]);

  manc_test_set3(symbols);
  result= manc_decode(symbols,bytes,&bytecount);
  Serial.printf("bytes (%d)",bytecount); for( int i=0; i<bytecount; i++ ) Serial.printf(" %02X",bytes[i]); Serial.printf(" %d/%s\n",result,manc_result_strings[result]);

  uint32_t t0=micros();
    bytecount= sizeof bytes; 
    for( int t=0; t<1000; t++) manc_decode(symbols,bytes,&bytecount);
  uint32_t t1=micros();
  Serial.printf("decoding in %ld ns (%ld ns per byte, transmission would take %d ns)\n", t1-t0, (t1-t0+2)/4, 80000/24 );
}

