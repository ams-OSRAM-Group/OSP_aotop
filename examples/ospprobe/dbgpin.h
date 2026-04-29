// dbgpin.h - header for debug/trace pins (to connect to logic analyzer to measure time on cores 0 and 1)
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
#ifndef _DBGPIN_H_
#define _DBGPIN_H_


// Clear/set GPIO pins via GPIO.out_w1tc and GPIO.out_w1ts, bypassing digitalWrite because too slow.
#include "soc/gpio_struct.h" 


// Pin numbers
#define DBGPIN_0_PIN  1 // yellow probe   hi: isr sem signals captured, lo: after posting in work queue  --  for core 0 
#define DBGPIN_1_PIN  2 // purple probe   hi: start decoding, lo: when decoding finished                 --  for core 1


// Configure the pins
void dbgpin_init();


// Control pins (fast; digitalWrite too slow)
#define dbgpin_0_hi()  do { GPIO.out_w1ts = 1UL << DBGPIN_0_PIN; } while(0)
#define dbgpin_0_lo()  do { GPIO.out_w1tc = 1UL << DBGPIN_0_PIN; } while(0)
#define dbgpin_1_hi()  do { GPIO.out_w1ts = 1UL << DBGPIN_1_PIN; } while(0)
#define dbgpin_1_lo()  do { GPIO.out_w1tc = 1UL << DBGPIN_1_PIN; } while(0)


#endif

