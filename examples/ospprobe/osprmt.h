// osprmt.h - header for capture OSP telegram timings using the RMT (Remote Control) block of the ESP32
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
#ifndef _OSPRMT_H_
#define _OSPRMT_H_


#include "manc.h"                        // manc_result_t 


#define OSPRMT_MAX_BYTES   12            // OSP telegram has max 12 bytes, want to take some margin, but RMT does not have more space


typedef struct osprmt_decodedtele_s {
  uint32_t      timestamp_us;            // Timestamp of reception based on micros()
  uint32_t      seqnum;                  // Sequence number; skips when there was a "buffer overflow" (all rawteles[] were in use)
  uint8_t       dir_tx;                  // Captured command telegramn on tx line (1) or response telegram on rx line (0)
  uint8_t       bytes[OSPRMT_MAX_BYTES]; // Decoded bytes of the telegram
  int           bytecount;               // Number of bytes in the decoded telegram bytes[]
  manc_result_t decoderesult;            // Was decoding succesful?
} osprmt_decodedtele_t;


void osprmt_init();
void osprmt_start();
int  osprmt_poll(osprmt_decodedtele_t*tele);


int osprmt_queue_free_count();
int osprmt_queue_work_count();


#endif
