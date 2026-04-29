// report.cpp - reporting ospprobe results - via OLED or USB/SERIAL
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


#include <Arduino.h>           // eg for Serial
#include <aoui32.h>            // toled_oled_init()
#include "dbgpin.h"            // dbgpin_1_hi()
#include "osprmt.h"            // osprmt_decodedtele_t
#include "ospprobe.h"          // BUT_ZERO
#include "report.h"            // self


// Record the main UI state: Load (oledload), Statistics (oledstat), Telegrams (oledtele) and USB Serial (vcomtele).
static report_mode_t report_mode;


// Helper to draw a 4 box dashboard; could have been part of aoui32; used for Statistics (oledstat).
static void report_dashboard(const char * lbl1, const char *  val1, const char * lbl2, const char *  val2, const char * lbl3, const char *  val3, const char * lbl4, const char *  val4 ) {
  toled_clear();

  // Boxes
  toled_fillrect(  0,  0,  62, 14 );
  toled_fillrect( 65,  0, 127, 14 );
  toled_fillrect(  0, 17,  62, 31 );
  toled_fillrect( 65, 17, 127, 31 );

  // Labels in tiny font
  toled_font(toled_font_mono5, TOLED_COL_BLACK );
  toled_cursor( 0+1, 0+0); toled_str(lbl1);
  toled_cursor(65+1, 0+0); toled_str(lbl2);
  toled_cursor( 0+1,17+0); toled_str(lbl3);
  toled_cursor(65+1,17+0); toled_str(lbl4);

  // Type value in smaller font (and reverse video)
  toled_font(toled_font_sans8, TOLED_COL_BLACK );
  toled_cursor( 0+3, 0+6); toled_str(val1);
  toled_cursor(65+3, 0+6); toled_str(val2);
  toled_cursor( 0+3,17+6); toled_str(val3);
  toled_cursor(65+3,17+6); toled_str(val4);

  toled_commit();
}


// [sn]printf is slow, stream to buffer `ostr` (output string) using these macros, used to make report_vcomtele_print() and report_oledtele_print() faster.
const char ostr_hex[]= {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
#define ostr_append_char(ch)     do { *(ostr++)=(ch); } while( 0 )
#define ostr_append_uint4(num)   do { *(ostr++)=ostr_hex[ (num) & 0xF ]; } while( 0 )
#define ostr_append_uint8(num)   do { ostr_append_uint4((num)>> 4); ostr_append_uint4((num)>> 0); } while( 0 )
#define ostr_append_uint12(num)  do { ostr_append_uint4((num)>> 8); ostr_append_uint4((num)>> 4); ostr_append_uint4((num)>>0); } while( 0 )
#define ostr_append_uint16(num)  do { ostr_append_uint4((num)>>12); ostr_append_uint4((num)>>8); ostr_append_uint4((num)>>4); ostr_append_uint4((num)>>0); } while( 0 )
#define ostr_append_uint24(num)  do { ostr_append_uint4((num)>>20); ostr_append_uint4((num)>>16); ostr_append_uint4((num)>>12); ostr_append_uint4((num)>>8); ostr_append_uint4((num)>>4); ostr_append_uint4((num)>>0); } while( 0 )
#define ostr_append_uint32(num)  do { ostr_append_uint4((num)>>28); ostr_append_uint4((num)>>24); ostr_append_uint4((num)>>20); ostr_append_uint4((num)>>16); ostr_append_uint4((num)>>12); ostr_append_uint4((num)>>8); ostr_append_uint4((num)>>4); ostr_append_uint4((num)>>0); } while( 0 )


// Print telegram to Serial (fast, ie using ostr_append_xxx)
static void report_vcomtele_print(osprmt_decodedtele_t * tele, bool seqnum_missed) {
  char buf[64];
  char * ostr=buf; // "output string"

  ostr_append_uint32(tele->timestamp_us);
  ostr_append_char( seqnum_missed ? '+' : ' ' );
  ostr_append_uint16(tele->seqnum);
  ostr_append_char( (tele->decoderesult==manc_result_ok)  ?  (tele->dir_tx?'T':'R')  :  (tele->dir_tx?'t':'r') ); // R/T is OK; r/t is decode errror
  if( tele->decoderesult==manc_result_ok ) {
    for( int i=0; i<tele->bytecount; i++) ostr_append_uint8(tele->bytes[i]);
  } else {
    ostr_append_uint32(tele->decoderesult); // uint8 is enough, but now 4 bytes matching the smallest telegram size, hopefully less anomalies in code
  }
  ostr_append_char('\n');
  ostr_append_char('\0');
  dbgpin_1_hi();
  Serial.write(buf); // Faster than printf
  dbgpin_1_lo();
}


// Print telegram to report_lines[] buffer (fast, ie using ostr_append_xxx);
// sets report_tele_dirty, which triggers a repaint by report_refresh_oledtele.
#define REPORT_TELE_LINE_LEN   33
#define REPORT_TELE_LINE_COUNT 30 // font is 6 pixels high 32/6=5 lines per screen, 6 screens
typedef struct report_tele_line_s {
  char line[REPORT_TELE_LINE_LEN];
} report_tele_line_t;
static report_tele_line_t report_tele_lines[REPORT_TELE_LINE_COUNT];
static int report_tele_line_count;
static bool report_tele_dirty;
static void report_oledtele_print(osprmt_decodedtele_t * tele, bool seqnum_missed) {
  if( report_tele_line_count>=REPORT_TELE_LINE_COUNT ) return;
  char * ostr= report_tele_lines[report_tele_line_count].line; // "output string"

  // 01234567890123456789012345678901
  // nnT 112233 445566778899AABB CC
  ostr_append_uint8(tele->seqnum);
  ostr_append_char( (tele->decoderesult==manc_result_ok)  ?  (tele->dir_tx?'T':'R')  :  (tele->dir_tx?'t':'r') ); // R/T is OK; r/t is decode errror
  ostr_append_char(' ');
  if( tele->decoderesult==manc_result_ok ) {
    int count= tele->bytecount-1;
    for( int i=0; i<=count; i++) {
      if( i==3 || i==count ) ostr_append_char(' ');
      ostr_append_uint8(tele->bytes[i]);
    }
  } else {
    ostr_append_uint8(tele->decoderesult); // uint8 is enough, but now 4 bytes like the smallerst telegram
  }
  ostr_append_char('\0');

  report_tele_line_count++;
  report_tele_dirty= true;
}


// Recording stats
static bool     report_tx_virgin;    // if true, no telegram seen on tx line (so report_tx_seqnum invalid)
static bool     report_rx_virgin;    // if true, no telegram seen on rx line (so report_rx_seqnum invalid)
static uint32_t report_tx_seqnum;    // seqnum of last tx telegram seen (only defined when not report_tx_virgin)
static uint32_t report_rx_seqnum;    // seqnum of last rx telegram seen (only defined when not report_rx_virgin)
static uint32_t report_tx_missed;    // number of missed tx telegrams; gap in seqnum (only defined when not report_tx_virgin)
static uint32_t report_rx_missed;    // number of missed rx telegrams; gap in seqnum (only defined when not report_rx_virgin)
static uint32_t report_tx_errors;    // number of tx telegrams with decode errors (only defined when not report_tx_virgin)
static uint32_t report_rx_errors;    // number of rx telegrams with decode errors (only defined when not report_rx_virgin)
static uint32_t report_load_busy_us; // number of us the lines was busy (with telegrams)

// When zero button is pressed, we record the zero position (and subtract that from the real stats above)
static uint32_t report_tx_seqnum_zero;
static uint32_t report_rx_seqnum_zero;
static uint32_t report_tx_missed_zero;
static uint32_t report_rx_missed_zero;
static uint32_t report_tx_errors_zero;
static uint32_t report_rx_errors_zero;


// Update stats based on telegram
// Take some physical actions: flips LEDTX/LEDRX, print over USB
#define REPORT_LEDTX_SPAN_MS 50
static uint32_t report_ledtx_prev_ms;
#define REPORT_LEDRX_SPAN_MS 50
static uint32_t report_ledrx_prev_ms;
void report_telegram(osprmt_decodedtele_t *tele) {
  bool seqnum_missed= false;
  // Collect stats for tx and rx
  if( tele->dir_tx ) {
    // Init tx stats when coming oput of virgin mode
    if( report_tx_virgin ) {
      // spoof "valid" previous seqnum
      report_tx_seqnum= tele->seqnum-1;
      report_tx_missed= 0;
      report_tx_errors= 0;
      // leaving tx virgin mode
      report_tx_virgin= false;
    }
    // Count missing tx telegrams
    if( tele->seqnum != report_tx_seqnum+1 ) {
      report_tx_missed += tele->seqnum - (report_tx_seqnum+1);
      seqnum_missed= true;
    }
    // Count erroneous tx telegrams
    if( tele->decoderesult != manc_result_ok ) {
      report_tx_errors += 1;
    }
    // Record last tx seqnum
    report_tx_seqnum= tele->seqnum;
    // Need to flip LEDTX
    uint32_t now= millis();
    if( now - report_ledtx_prev_ms > REPORT_LEDTX_SPAN_MS ) { aoui32_led_toggle(LED_TX); report_ledtx_prev_ms= now; }
  } else {
    // Init rx stats when coming oput of virgin mode
    if( report_rx_virgin ) {
      // spoof "valid" previous seqnum
      report_rx_seqnum= tele->seqnum-1;
      report_rx_missed= 0;
      report_rx_errors= 0;
      // leaving rx virgin mode
      report_rx_virgin= false;
    }
    // Count missing rx telegrams
    if( tele->seqnum != report_rx_seqnum+1 ) {
      report_rx_missed += tele->seqnum - (report_rx_seqnum+1);
      seqnum_missed= true;
    }
    // Count erroneous rx telegrams
    if( tele->decoderesult != manc_result_ok ) {
      report_rx_errors += 1;
    }
    // Record last rx seqnum
    report_rx_seqnum= tele->seqnum;
    // Need to flip LEDRX
    uint32_t now= millis();
    if( now - report_ledrx_prev_ms > REPORT_LEDRX_SPAN_MS ) { aoui32_led_toggle(LED_RX); report_ledrx_prev_ms= now; }
  }

  // Accumulate time spend on telegrams. Each bit takes 1/2.4us. Add 1 to round up, add 8 as inter telegram dead time
  if( tele->decoderesult==manc_result_ok ) report_load_busy_us += tele->bytecount*80/24 + 1 + 8;
  else report_load_busy_us += 6*80/24 + 1 + 8; // assume 6 bytes in missed telegram

  // Note, with OLED updates every 500ms (REPORT_REFRESH_OLEDTELE_SPAN_MS), there could
  // been 0.5 * 2.4E6 = 1 200 000 bits = 150 000 bytes = 12500 (12 byte) telegrams

  // If mode is oledtele print the tele to the OLED string buffer (which set report_tele_dirty, which triggers a repaint by report_refresh_oledtele)
  if( report_mode==report_mode_oledtele ) report_oledtele_print(tele,seqnum_missed);

  // If mode is vcomtele print the tele on serial-over-USB
  if( report_mode==report_mode_vcomtele ) report_vcomtele_print(tele,seqnum_missed);
}


// Helpers for report_refresholed() when mode is Load (oledload).
#define REPORT_REFRESH_OLEDLOAD_SPAN_MS 500
static uint32_t report_refresh_oledload_prev_ms;
static void report_refresh_oledload() {
  uint32_t now= millis();
  if( now - report_refresh_oledload_prev_ms < REPORT_REFRESH_OLEDLOAD_SPAN_MS )  return;

  // Compute the load
  uint32_t span_ms = now - report_refresh_oledload_prev_ms;
  float load_perc = report_load_busy_us / span_ms / 10.0f;
  // Reset load counters
  report_load_busy_us= 0;
  report_refresh_oledload_prev_ms = now;

  // Compute (zerod) seqnums
  uint32_t txseqnum= report_tx_seqnum-report_tx_seqnum_zero;
  uint32_t rxseqnum= report_rx_seqnum-report_rx_seqnum_zero;

  // Refresh OLED
  char abuf[16],xbuf[16],ybuf[16];
  static int flip;
  snprintf(abuf,32,"%.1f %%",load_perc);
  snprintf(xbuf,32,"%lu",txseqnum);
  snprintf(ybuf,32,"%lu",rxseqnum);
  aoui32_oled_state(abuf,xbuf,ybuf, (flip++%2)?"load":"", txseqnum<=999999999?"t":"", rxseqnum<=999999999?"r":"");
}


// Helpers for report_refresholed() when mode is Statistics (oledstat).
#define REPORT_REFRESH_OLEDSTAT_SPAN_MS 500
static uint32_t report_refresh_oledstat_prev_ms;
static void report_refresh_oledstat() {
  uint32_t now= millis();
  if( now - report_refresh_oledstat_prev_ms < REPORT_REFRESH_OLEDSTAT_SPAN_MS )  return;
  report_refresh_oledstat_prev_ms = now;

  // The refresh
  #define REPORT_VAL_SIZE   16
  char val_tm[REPORT_VAL_SIZE]; if( report_tx_virgin ) snprintf(val_tm,REPORT_VAL_SIZE,"-"); else snprintf(val_tm,REPORT_VAL_SIZE,"%lu",report_tx_missed-report_tx_missed_zero);
  char val_td[REPORT_VAL_SIZE]; if( report_tx_virgin ) snprintf(val_td,REPORT_VAL_SIZE,"-"); else snprintf(val_td,REPORT_VAL_SIZE,"%lu",report_tx_errors-report_tx_errors_zero);
  char val_rm[REPORT_VAL_SIZE]; if( report_rx_virgin ) snprintf(val_rm,REPORT_VAL_SIZE,"-"); else snprintf(val_rm,REPORT_VAL_SIZE,"%lu",report_rx_missed-report_rx_missed_zero);
  char val_rd[REPORT_VAL_SIZE]; if( report_rx_virgin ) snprintf(val_rd,REPORT_VAL_SIZE,"-"); else snprintf(val_rd,REPORT_VAL_SIZE,"%lu",report_rx_errors-report_rx_errors_zero);
  report_dashboard("tm",val_tm, "rm",val_rm, "td",val_td, "rd",val_rd );
}


// Helpers for report_refresholed() when mode is Telegrams (oledtele).
#define REPORT_REFRESH_OLEDTELE_SPAN_MS 500
static uint32_t report_refresh_oledtele_prev_ms;
static int report_tele_page;
static void report_refresh_oledtele() {
  uint32_t now= millis();
  if( now - report_refresh_oledtele_prev_ms < REPORT_REFRESH_OLEDTELE_SPAN_MS )  return;
  report_refresh_oledtele_prev_ms = now;

  // The refresh
  if( !report_tele_dirty ) return;

  toled_clear();
    toled_font(toled_font_mono5, TOLED_COL_WHITE );
    int first= report_tele_page*5;
    int last= min( report_tele_page*5+5, report_tele_line_count );
    for(int i=first, y=1; i<last; i++,y+=6 ) {
      toled_cursor( 0, y); toled_str(report_tele_lines[i].line);
    }
    // Serial.printf("page=%d\n",report_tele_page);
    toled_openrect(120,  0, 127, 31 );
    toled_fillrect(122, 1+5*report_tele_page, 125, 4+5*report_tele_page );
  toled_commit();

  report_tele_dirty= false;
}


// Refresh OLED depending on state
void report_refresholed() {
  switch( report_mode ) {
    case report_mode_oledload: report_refresh_oledload(); break; // Load
    case report_mode_oledstat: report_refresh_oledstat(); break; // Statistics
    case report_mode_oledtele: report_refresh_oledtele(); break; // Telegrams
    case report_mode_vcomtele: /*skip*/                   break; // USB Serial has no refresh, directly streamed
    default                  : Serial.printf("ERROR: unknown report mode %d\n", report_mode ); break;
  }
}


// Scan buttons and update state if a button is pressed
void report_handlebuttons() {
  // Check for button press to change mode
  aoui32_but_scan();

  // Mode
  if( aoui32_but_wentdown(BUT_MODE) ) {
    report_mode_setnext();
  }

  // Zero
  if( aoui32_but_wentdown(BUT_ZERO) ) {
    switch( report_mode ) {
      case report_mode_oledload: report_tx_seqnum_zero= report_tx_seqnum; report_rx_seqnum_zero= report_rx_seqnum; break;
      case report_mode_oledstat: report_tx_missed_zero= report_tx_missed; report_rx_missed_zero= report_rx_missed; report_tx_errors_zero= report_tx_errors; report_rx_errors_zero= report_rx_errors; break;
      case report_mode_oledtele: report_tele_line_count=0; report_tele_dirty=true; report_tele_page=0; break;
      case report_mode_vcomtele: /*skip*/ break;
      default                  : Serial.printf("ERROR: unknown report mode %d\n", report_mode ); break;
    }
  }

  // Next
  if( aoui32_but_wentdown(BUT_NEXT) ) {
    if( report_tele_line_count > (report_tele_page+1)*5 ) report_tele_page++; else report_tele_page=0;
    report_tele_dirty=true;
  }

}


// Init report modle
void report_init() {
  // Init underlying ui
  aoui32_led_init (LED_TX_PIN  , LED_RX_PIN  );
  aoui32_but_init (BUT_MODE_PIN, BUT_NEXT_PIN, BUT_ZERO_PIN);
  aoui32_oled_init(OLED_SDA_PIN, OLED_SCL_PIN);

  // Init reporting state
  report_mode= report_mode_oledload;
  report_tx_virgin= true;
  report_rx_virgin= true;
  report_load_busy_us=0;

  Serial.printf("report: init\n");
}


// Get current mode
report_mode_t report_mode_get() {
  return report_mode;
}


// Change current mode
void report_mode_set(report_mode_t mode) {
  report_mode= mode;

  // Init that mode
  switch( report_mode ) {
    case report_mode_oledload: report_refresh_oledload_prev_ms= millis() - REPORT_REFRESH_OLEDLOAD_SPAN_MS; break;
    case report_mode_oledstat: report_refresh_oledstat_prev_ms= millis() - REPORT_REFRESH_OLEDSTAT_SPAN_MS; break;
    case report_mode_oledtele: report_refresh_oledtele_prev_ms= millis() - REPORT_REFRESH_OLEDTELE_SPAN_MS; report_tele_line_count=0; report_tele_dirty=true; report_tele_page=0; break;
    case report_mode_vcomtele: /* no (OLED)  refreshes */ aoui32_oled_word("USB Serial"); break;
    default                  : Serial.printf("ERROR: unknown report mode %d\n", report_mode ); break;
  }
}


// Change to "next" mode
void report_mode_setnext() {
  switch( report_mode ) {
    case report_mode_oledload: report_mode_set(report_mode_oledstat); break;
    case report_mode_oledstat: report_mode_set(report_mode_oledtele); break;
    case report_mode_oledtele: report_mode_set(report_mode_vcomtele); break;
    case report_mode_vcomtele: report_mode_set(report_mode_oledload); break;
    default                   : Serial.printf("ERROR: unknown report mode %d\n", report_mode ); break;
  }
}

