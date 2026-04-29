// report.h - reporting ospprobe results - via OLED or USB/SERIAL
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
#ifndef _REPORT_H_
#define _REPOR_H_


// === UI Pinning ===========================================================


//         PINOUT ESP32-S3 supermini
//                +-----+++-----+
//           x--TX| LED USB LED |5V----------ERNI
//           x--RX|  48     bat |GND---------ERNI
//      TRACE0---1|             |3V3---------lvlshft/oled
//      TRACE1---2|             |13--NEXT----button
// LED---LEDTX---3|             |12--ZERO----button
// LED---LEDRX---4|             |11--MODE----button
//           x---5|       RGB   |10--x
// OLED----SDA---6|        48   |9---OSPRX---ERNI
// OLED----SCL---7|             |8---OSPTX---ERNI
//                +-------------+


// LED aliases for OSPprobe board
#define LED_TX        AOUI32_LED_GRN
#define LED_RX        AOUI32_LED_RED
// LED pin numbers for OSPprobe board
#define LED_TX_PIN    3
#define LED_RX_PIN    4


// Button aliases for OSPprobe board
#define BUT_MODE      AOUI32_BUT_A
#define BUT_ZERO      AOUI32_BUT_X
#define BUT_NEXT      AOUI32_BUT_Y
// Button pin numbers for OSPprobe board
#define BUT_MODE_PIN  11
#define BUT_NEXT_PIN  12
#define BUT_ZERO_PIN  13


// OLED pin numbers for OSPprobe board
#define OLED_SDA_PIN  6
#define OLED_SCL_PIN  7


// === Main API =============================================================


typedef enum report_mode_e {
  report_mode_oledload,
  report_mode_oledstat,
  report_mode_oledtele,
  report_mode_vcomtele,
} report_mode_t;


report_mode_t report_mode_get();
void          report_mode_set(report_mode_t mode);
void          report_mode_setnext();


void          report_telegram(osprmt_decodedtele_t *tele);
void          report_handlebuttons();
void          report_refresholed();


void          report_init();


#endif

