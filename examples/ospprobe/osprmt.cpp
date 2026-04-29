// osprmt.cpp - capture OSP telegram timings using the RMT (Remote Control) block of the ESP32
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


// For documentation see 
// - API in   https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/rmt.html
// - block in https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf#rmt 


#include <Arduino.h>           // eg for Serial
#include "driver/rmt_rx.h"     // eg for rmt_symbol_word_t. Note driver/rmt.h is deprecated, use driver/rmt_tx.h or driver/rmt_rx.h instead.
#include "esp_task_wdt.h"      // esp_task_wdt_delete(), disableCore0WDT
#include "dbgpin.h"            // e.g. for dbgpin_0_hi
#include "osprmt.h"            // self 

// Monitored OSP pin
#define OSPRMT_PIN_CMD      8                                    // Receive Manchester encoded signal on this pin; this pin is SIO-P line for commands "TX"
#define OSPRMT_PIN_RES      9                                    // Receive Manchester encoded signal on this pin; this pin is SIO-N line for responses "RX"


// Timing constants
#define OSPRMT_FREQ_HZ     19200000                              // OSP bit rate of 2.4MBit/s, means 8 RMT ticks per bit
#define OSPRMT_TICK_NS     (1000*1000*1000/OSPRMT_FREQ_HZ)       // RMT tick time is 52.083333 ns


// Max counts
#define OSPRMT_MAX_SYMBOLS (8*OSPRMT_MAX_BYTES)                  // Need _max_ one RMT symbol per bit. A symbol is one low and one high period each with its duration. 
// ESP32S3 has 4 TX channels and 4 RX channels. The RMT (in S3) has 384 words for symbols, which is 48 per channel.
// One channel can use the words from another channel, but it appears not accross the TX/RX split.
// Channels 0,1,2,3 are for TX, and there are 48+48+48+48 words
// Channels 4,5,6,7 are for RX, and we use one channel with 48+48 and another with also 48+48 words.
// 48+48 = 96 words for symbols, each symbol for one bit, so that cover 8 bits x 12 bytes per telegram.
#define OSPRMT_MAX_TELES   256                                   // Max number of telegrams that can be pending pending (received but not yet processed; "buffering")
// Shortest telegram is 4 bytes or 32 bits or 13 us, add 8 us idle: max throughput is one telegram every 21 us, in practice every 35us.
// One OLED update is about 6500 us or 6500/35=185 telegrams.


// Telegram is received as an array of RMT symbols, from the ESP32's RMT block. 
// When captured, this driver adds the timestamp, and how many telegrams were skipped (if buffer overflowed).
// This data is called a raw telegram ("rawtele") in this code.
// Note: an rmt_symbol_word_t is 32 bits (an ESP32 "word"?), split in two 16 bit units.
// Each unit consists of a level (1 bit) and a duration (15 bit).
// In this code we equate the term "symbol" and "rmt_symbol_word_t" to be two units.
typedef struct osprmt_rawtele_s {
  uint32_t          timestamp_us;                                // Timestamp of reception using micros()
  uint32_t          seqnum;                                      // Sequence number; skips when all rawteles[] were in use ("buffer overflow")
  rmt_symbol_word_t symbols[OSPRMT_MAX_SYMBOLS];                 // Captured symbols (=level and duration)
  uint8_t           dir_tx;                                      // Captured command telegramn on tx line (1) or response telegram on rx line (0)
} osprmt_rawtele_t;
// Size of one osprmt_rawtele_t is 4 + 4 + 4*8*12 + 4 = 396 bytes
// 396 Bytes * 256 Poolsize = 99 kiB


// Buffers for captured telegrams
static osprmt_rawtele_t       osprmt_rawteles[OSPRMT_MAX_TELES]; // Pool of raw telegrams (raw because timestamps, not bits)
static osprmt_rawtele_t       osprmt_rawteleoverflow_cmd;        // One extra raw telegram (used when queue_free is depleted) for command capturing
static osprmt_rawtele_t       osprmt_rawteleoverflow_res;        // One extra raw telegram (used when queue_free is depleted) for response capturing
static volatile int IRAM_ATTR osprmt_flag_captured_cmd;          // Flag set by ISR when RMT captured a command. IRAM because used in ISR. Semaphore is neater (no watchdog issues, but 8us slower)
static volatile int IRAM_ATTR osprmt_flag_captured_res;          // Flag set by ISR when RMT captured a response. IRAM because used in ISR. Semaphore is neater (no watchdog issues, but 8us slower)
static TaskHandle_t           osprmt_taskhandle;                 // Handle to the newly created task.
static QueueHandle_t          osprmt_queue_free;                 // The queue with unused/empty raw telegrams (to be used for a next RMT capture)
static QueueHandle_t          osprmt_queue_work;                 // The queue with captured raw telegrams (to be processed)
static rmt_channel_handle_t   osprmt_channel_cmd;                // The RMT channel (handle for RMT driver) for commands
static rmt_channel_handle_t   osprmt_channel_res;                // The RMT channel (handle for RMT driver) for responses
static uint32_t               osprmt_seqnum_cmd;                 // The number of received command telegrams, used to fill the seqnum in rawteles[]
static uint32_t               osprmt_seqnum_res;                 // The number of received response telegrams, used to fill the seqnum in rawteles[]
// Step sequence number, which wraps.
// Shortest (4 bytes) telegram is about 13⅓us, there is a mandatory wait 
// of about 8us after each telegram. Fastest rate is to send about one 
// telegram per 21⅓us, so 2^32 telegrams take at least 91625s or 25 hours.


// RMT reception configuration, needed for rmt_receive()
static rmt_receive_config_t osprmt_receive_config = {
  .signal_range_min_ns = 5,    // Filter out spikes < 5ns (note 5ns is 10% of OSPRMT_TICK_NS)
  .signal_range_max_ns = 1000, // When line is idle this long (1000ns), telegram is assumed at end (note, one bit is 833ns)
  .flags               = 0,
};


// ISR called by RMT block when captured finished (idle for at least signal_range_max_ns)
// - The callback itself and functions called by it should be placed in IRAM.
// - The variables used in the function should be in the SRAM as well. 
// - Reason: _external_ RAM or Flash must be pulled in via a quadspi fetcher, which might be in locked state when ISR fires.
// - The ESP32S3 has two cores, both capable to handle any interrupt. 
//   Interrupts are routed (from a peripheral like RMT) via an "interrupt matrix" to either core.
//   By default the matrix is configured to have the interrupt routed to the core that registers the interrupt.
// - It seems allowed to call rmt_receive in the ISR (but not implemented)
//   https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/rmt.html#thread-safety
// - Using a semaphore or vTaskNotifyGiveFromISR() is cleaner: no need to disable watchdogs.
//   But time from end of bit train to xQueueSend increases from 7 us to 14 us.
static bool IRAM_ATTR osprmt_callback_cmd(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data) {
  dbgpin_0_hi();
    osprmt_flag_captured_cmd= 1;
  dbgpin_0_lo();
  // Return true if a context switch is requested when interrupt exits.
  return pdFALSE;
}
static bool IRAM_ATTR osprmt_callback_res(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data) {
  dbgpin_0_hi();
    osprmt_flag_captured_res= 1;
  dbgpin_0_lo();
  // Return true if a context switch is requested when interrupt exits.
  return pdFALSE;
}


// Run a task on core 0 to continuously arm the RMT block for capturing.
// Note: the ESP32 core 0 normally only runs WiFi tasks. The user app is on core 1.
// When not using WiFi, core 0 is mostly idling.
// This is the task function mapped to core 0.
// It performs setup of RMT and runs the loop that constantly captures telegrams.
void osprmt_task_core0( void * pvParameters ) {
  // Create RMT channel configuration
  // Hacky pattern: 'struct s={}' creates struct with all zeros, to avoid 'missing initializer' for members in a new version.
  rmt_rx_channel_config_t cfg = {};  
  // Next, initialize the fields we "know"
  cfg.clk_src = RMT_CLK_SRC_DEFAULT;
  cfg.resolution_hz = OSPRMT_FREQ_HZ;
  cfg.intr_priority = 3; // The ESP32-S3 supports 7 priority levels, 1 is lowest; https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/hlinterrupts.html
  cfg.flags.with_dma = 0; // When 0, uses RMT's own memory. 
  cfg.mem_block_symbols = OSPRMT_MAX_SYMBOLS;

   // Create RMT channel for OSP commands
  cfg.gpio_num = (gpio_num_t)OSPRMT_PIN_CMD;
  ESP_ERROR_CHECK( rmt_new_rx_channel(&cfg, &osprmt_channel_cmd) );

  // Create RMT channel for OSP responses
  cfg.gpio_num = (gpio_num_t)OSPRMT_PIN_RES;
  ESP_ERROR_CHECK( rmt_new_rx_channel(&cfg, &osprmt_channel_res) );

  // Register callback (called from RMTs ISR) when capture completed - for both channels
  rmt_rx_event_callbacks_t cbs = {};
  cbs.on_recv_done = osprmt_callback_cmd;
  ESP_ERROR_CHECK( rmt_rx_register_event_callbacks(osprmt_channel_cmd, &cbs, NULL) );
  cbs.on_recv_done = osprmt_callback_res;
  ESP_ERROR_CHECK( rmt_rx_register_event_callbacks(osprmt_channel_res, &cbs, NULL) );

  // Enable both channels
  ESP_ERROR_CHECK( rmt_enable(osprmt_channel_cmd) );
  ESP_ERROR_CHECK( rmt_enable(osprmt_channel_res) );

  // Inits for the loop
  osprmt_rawtele_t * rawtele_cmd;
  osprmt_rawtele_t * rawtele_res;
  int captured_cmd= 1; // spoof capturing, this causes an initial arming for cmd
  int captured_res= 1; // spoof capturing, this causes an initial arming for res
  osprmt_seqnum_cmd= 0;
  osprmt_seqnum_res= 0;

  // Wait for trigger to start looping
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  // Continuous arming of RMT
  for(;;) {
    // Arm the RMT block for reception of command or response
    dbgpin_0_hi();
    if( captured_cmd ) {
      // Get a raw telegram from the free queue (but don't wait if there's none). Note: We pass &rawtele because the queue stores the pointer itself
      if( xQueueReceive(osprmt_queue_free, &rawtele_cmd, 0)==pdPASS ) {
        // We got a rawtele from the free queue. 
      } else {
        // The free queue was empty, use the overflow telegram. 
        rawtele_cmd= &osprmt_rawteleoverflow_cmd;
      }
      // Reset capture completion flag
      osprmt_flag_captured_cmd=0;
      // Arm the RMT block to start listening for telegrams to capture.
      ESP_ERROR_CHECK( rmt_receive(osprmt_channel_cmd, rawtele_cmd->symbols, sizeof rawtele_cmd->symbols, &osprmt_receive_config) );
    }

    if( captured_res ) {
      // Get a raw telegram from the free queue (but don't wait if there's none). Note: We pass &rawtele because the queue stores the pointer itself
      if( xQueueReceive(osprmt_queue_free, &rawtele_res, 0)==pdPASS ) {
        // We got a rawtele from the free queue. 
      } else {
        // The free queue was empty, use the overflow telegram. 
        rawtele_res= &osprmt_rawteleoverflow_res;
      }
      // Reset capture completion flag
      osprmt_flag_captured_res=0;
      // Arm the RMT block to start listening for telegrams to capture.
      ESP_ERROR_CHECK( rmt_receive(osprmt_channel_res, rawtele_res->symbols, sizeof rawtele_res->symbols, &osprmt_receive_config) );
     }
    dbgpin_0_lo();

    // Wait till ISR signals capture complete. This busy-wait is the reason to shut down the watchdog
    while( !osprmt_flag_captured_cmd && ! osprmt_flag_captured_res ) /*skip*/;
    // Take snapshot, because the flags are inspected twice in the loop
    captured_cmd= osprmt_flag_captured_cmd;
    captured_res= osprmt_flag_captured_res;

    // Enqueue the captured telegram for processing in other task
    dbgpin_0_hi();
    if( captured_cmd ) {
      if( rawtele_cmd==&osprmt_rawteleoverflow_cmd ) {
        // The rawtele was the overflow buffer, so we can't report it.
      } else {
        // If rawtele was an actual buffer, queue it for processing.
        // Record timestamp.
        rawtele_cmd->timestamp_us= micros();
        // Record seqnum (to show how many raw telegrams were missed).
        rawtele_cmd->seqnum = osprmt_seqnum_cmd;
        // Record direction
        rawtele_cmd->dir_tx = 1;
        // Queue raw telegram for processing. Again, note the &.
        xQueueSend(osprmt_queue_work, &rawtele_cmd, portMAX_DELAY);
      }
      osprmt_seqnum_cmd++;
    }
    if( captured_res ) {
      if( rawtele_res==&osprmt_rawteleoverflow_res ) {
        // The rawtele was the overflow buffer, so we can't report it.
      } else {
        // If rawtele was an actual buffer, queue it for processing.
        // Record timestamp.
        rawtele_res->timestamp_us= micros();
        // Record seqnum (to show how many raw telegrams were missed).
        rawtele_res->seqnum = osprmt_seqnum_res;
        // Record direction
        rawtele_res->dir_tx = 0;
        // Queue raw telegram for processing. Again, note the &.
        xQueueSend(osprmt_queue_work, &rawtele_res, portMAX_DELAY);
      }
      osprmt_seqnum_res++;
    }
    dbgpin_0_lo();
  }
}


// Creates a task for RMT on core 0
// Core 0 is normally used for WiFi, but in iur application it would be doing nothing.
// We create one task (osprmt_task_core0) to capture the command and response telegrams.
// By default there is an "idle" task on core 0. 
void osprmt_init() {
  // Create queues: one with empty rawteles ("free", ready to be used for a capture), and one with captured telegrams ("work").
  // Note that the queues store _pointers_ to raw telegrams.
  osprmt_queue_free = xQueueCreate(OSPRMT_MAX_TELES, sizeof(osprmt_rawtele_t*) );
  osprmt_queue_work = xQueueCreate(OSPRMT_MAX_TELES, sizeof(osprmt_rawtele_t*) );
  if( osprmt_queue_free==0 || osprmt_queue_work==0 ) Serial.printf("queue init failed\n");

  // Fill the free queue with all raw telegrams from osprmt_rawteles[].
  for( int i=0; i<OSPRMT_MAX_TELES; i++ ) {
    osprmt_rawtele_t * rawtele= &osprmt_rawteles[i];
    // Note FreeRTOS copies the data at location &rawtele
    xQueueSend(osprmt_queue_free, &rawtele, portMAX_DELAY);
  }

  xTaskCreatePinnedToCore(
    osprmt_task_core0,   // Task function
    "osprmt_task_core0", // Name of task
    10000,               // Stack size of task
    NULL,                // parameter of the task
    20,                  // priority of the task (by default there are configMAX_PRIORITIES=25 priority levels; lowest is 0)
    &osprmt_taskhandle,  // Task handle to keep track of created task
    0);                  // pin task to core 0

  // End of setup
  Serial.printf("osprmt: init (pool %d teles a %d byte)\n", OSPRMT_MAX_TELES, sizeof(osprmt_rawtele_t) );
}


void osprmt_start() {
  // Unsubscribe the capture task from the watchdog; needed since it does unbounded while(); 
  // esp_task_wdt_delete(osprmt_taskhandle); // not needed: new tasks are not subscribed to the watchdog by default

  // Unsubscribes IDLE0 from the watchdog. This is needed because task IDLE0 will no longer run: osprmt_task_core0 will busy wait 100%, never yielding.
  disableCore0WDT(); 

  // Start capturing
  xTaskNotifyGive(osprmt_taskhandle);
}


int osprmt_poll(osprmt_decodedtele_t*tele) {
  // `captured` indicates if this function returns (in out param `tele`) a captured telegram
  int captured=0;

  // Get a raw telegram from the work queue (but don't wait if there's none)
  osprmt_rawtele_t * rawtele;
  if( xQueueReceive(osprmt_queue_work, &rawtele, 0)==pdPASS ) {
    dbgpin_1_hi();

      // Copy fields
      tele->timestamp_us = rawtele->timestamp_us;
      tele->seqnum = rawtele->seqnum;
      tele->dir_tx = rawtele->dir_tx;

      // Decode data in rawtele
      tele->bytecount= sizeof tele->bytes; // on input indicates bytes[] size, on output decoded length
      tele->decoderesult = manc_decode(rawtele->symbols, tele->bytes, &tele->bytecount);

      // Return rawtele to the free queue
      xQueueSend(osprmt_queue_free, &rawtele, portMAX_DELAY);

      // Flag capture happened
      captured= 1;

    dbgpin_1_lo();
  }

  // Return flag
  return captured; 
}


int osprmt_queue_free_count() {
  return uxQueueMessagesWaiting(osprmt_queue_free);
}


int osprmt_queue_work_count() {
  return uxQueueMessagesWaiting(osprmt_queue_work);
}