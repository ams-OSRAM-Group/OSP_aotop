// manc.h - Manchester decoder for OSP (convert bit flip time stamps to bits)
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
#ifndef _MANC_H_
#define _MANC_H_


#include "driver/rmt_rx.h"  // for rmt_symbol_word_t[]


// Errors returned by the decoder`
typedef enum manc_result_e {
  manc_result_ok = 0,         // Decode was successful (no check an preamble or CRC yet)
  manc_result_wronglevel,     // First pulse is not 0 (started capturing mid telegram?)
  manc_result_expectshort,    // Long pulse at start of a bit (must be short one)
  manc_result_wrongduration,  // Pulse length out of spec (3/4/5, 6/7/8/9/10)
  manc_result_toomanybytes,   // Number of (input) symbols too big to fit in (output) bytes[]
  manc_result_no8fold,        // Decoded pulses form bit sequence which is not an 8-fold (i.e. bytes)
  manc_result_assert,         // Assertion in the code (i.e. software produces unexpected values)
} manc_result_t;


// Convert a decoder result to a string
const char* manc_result_str( manc_result_t result );


// Decode an RMT symbols string to a bytes array
manc_result_t manc_decode(const rmt_symbol_word_t *symbols, uint8_t *bytes, int *bytecount );


// A simple test of the decoder
void manc_test();


#endif

