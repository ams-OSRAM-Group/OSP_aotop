// ospprobe.ino - application to tap the telegrams running over an OSP chain
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
#include <aoui32.h>       // aoui32_led_init(), aoui32_but_init(), toled_oled_init()
#include "dbgpin.h"       // dbgpin_init(), dbgpin_1_hi()
#include "osprmt.h"       // osprmt_init(), osprmt_poll()
#include "report.h"       // report_init()
#include "ospprobe.h"     // OSPPROBE_BANNER, BUT_MODE, OLED_SDA_PIN


// In Arduino IDE:
// - ESP32S3 Dev Module
// - Tools > USB CDC On Boot: Enabled (uses native USB block, baud rate irrelevant)
// - USB cable to PCB connector tagged "USB"


// See readme.me for user manual and optionally Wireshark setup


void setup() {
  // Identify over Serial  
  Serial.begin(115200); // Tools > USB CDC On Boot: Enabled (then baudrate is irrelevant)
  delay(1500); Serial.printf(OSPPROBE_BANNER1); delay(100); Serial.printf(OSPPROBE_BANNER2); delay(100); // Throttle printing
  Serial.printf("%s - version %s\n\n", OSPPROBE_LONGNAME, OSPPROBE_VERSION);

  // Initialize all libraries
  dbgpin_init();
  osprmt_init();
  report_init(); // Includes aoui32 (oled, buttons, leds)
  
  // Show end of init
  Serial.printf("app   : init (on core %d)\n\n",xPortGetCoreID() );
  aoui32_oled_splash(OSPPROBE_LONGNAME,OSPPROBE_VERSION); 
  delay(1000);

  // Start capturing
  osprmt_start();
  Serial.printf("send any char to switch to 'USB Serial'\n\n");
}


void loop() {
  // Let report handle button presses
  report_handlebuttons();

  // Mode change request via Serial
  if( Serial.read()!=-1 ) {
    report_mode_set(report_mode_vcomtele);
  }

  // Did core 0 capture a telegram in the background?
  osprmt_decodedtele_t tele;
  int captured= osprmt_poll(&tele);
  // If so, pass it to the reporting module
  if( captured ) report_telegram(&tele);

  // Let the report refresh the OLED
  report_refresholed();
}
