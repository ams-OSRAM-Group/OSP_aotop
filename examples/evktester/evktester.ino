// evktester.ino - test runner for EVK test cases
/*****************************************************************************
 * Copyright 2025 by ams OSRAM AG                                            *
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
#include "esp_mac.h"
#include <aospi.h>
#include <aoosp.h>
#include <aoui32.h>
#include <cases.h>
#include <evktester.h>


/*
DESCRIPTION
This application tests "all" hardware components in the Arduino OSP EVK. 
The application runs several tests in sequence. The tests are interactive.
Typically, the operator presses the A button to start the test.  During the 
test, the operator either performs a test action sensed by the application 
(eg pressing INT button) or the operator acknowledges an application test 
action (confirm all LEDs are white, by pressing Y, or X if not) or the
operator waits for the application to run and check the action (check I2C
EEPROM). Finally the users moves to the next test (by pressing Y).
In the mean time, a test report is printed to Serial.
For more details, see "USERMANUAL for the EVKtester" below.

HARDWARE
All hardware components are tested, so make sure they are available.
Note that it is possible to skip tests (just press Y multiple times).
The test expects the following topology:
  OPS32...SAIDbasic...SAIDlooker...CAN...CAN...RGBIstrip...terminator1
             |
     bouncingblock-EEPROM
However during the test small modifications must be made by the operator,
like replacing terminator1 by terminator2 or by a loop-back cable, or
replacing the bouncingblock-EEPROM by the colormix-EEPROM.
The SAIDsense board has been added, but the V2 version, not yet the v5 (todo).
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
During the test, all RGB LEDs, OSP32 signaling LEDs and SAIBasic indicator 
LEDs will be switched on and off. One test is not scripted, pressing the 
RST button on OSP32. The "end test" instructs to do that.
It is suggested to save the output - up to and including the RST at the 
end as a text file to document the test results. Note that the mac address
of the ESP32 is part of this test report, to link report to hardware.

OUTPUT
 ________      ___  ___            _
|  ____\ \    / / |/ / |          | |
| |__   \ \  / /| ' /| |_ ___  ___| |_ ___ _ __
|  __|   \ \/ / |  < | __/ _ \/ __| __/ _ \ '__|
| |____   \  /  | . \| ||  __/\__ \ ||  __/ |
|______|   \/   |_|\_\\__\___||___/\__\___|_|
EVKtester - version 1.0

spi: init(MCU-B)
osp: init
ui32: init

version: 1.0
cases: 18
mac: C0:4E:30:0A C0:F4:CA:3F

START
1.uibut= OK
2.uiled= OK
3.usbpow= OK
4.topo1= OK
5.term2= OK
6.loop= OK
7.white= OK
8.osp32i2c= OK
9.ioxled= OK
10.ioxbut= OK
11.rainbow= OK
12.bouncingblock= OK
13.colormix= OK
14.topo2= OK
15.senled= OK
16.sensen= OK
exec    : 16
running : 0
success : 16
fail    : 0
timeout : 0
END

ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce2820,len:0x1188
load:0x403c8700,len:0x4
load:0x403c8704,len:0xbf0
load:0x403cb700,len:0x30e4
entry 0x403c88ac

 ________      ___  ___            _
|  ____\ \    / / |/ / |          | |
| |__   \ \  / /| ' /| |_ ___  ___| |_ ___ _ __
|  __|   \ \/ / |  < | __/ _ \/ __| __/ _ \ '__|
| |____   \  /  | . \| ||  __/\__ \ ||  __/ |
|______|   \/   |_|\_\\__\___||___/\__\___|_|
EVKtester - version 1.0

spi: init(MCU-B)
osp: init
ui32: init

version: 1.0
cases: 18
mac: C0:4E:30:0A C0:F4:CA:3F
*/


/*
USERMANUAL for the EVKtester
- See also evktester.drawio.png
- Flash this sketch in the ESP32
- Keep the SerialMonitor open while running; it prints the test report.

- The OLED shows a test *description* in the form of "a.bbb: cccc cccc";
  the colon ":" identifies this as description.
- In the description "a" is test id/index, "bbb" is test name, and 
  "cccc cccc" describes what is going to happen, this might include 
  configuration instructions for the operator.
- Press X or Y to go to a previous (X) or next (Y) test; as shown
  by its description. Best is to run tests in order because a previous
  test could have wiring instructions also for a next test.
- Pressing A at a description will execute the test. So make sure
  to press that after completing any configuration instruction in 
  the description.
- After A is a pressed, the OLED will show a *run prompt* of the form
  "a.bbb> running". The greater than ">" symbol identifies that the test 
  is running.
- Some tests run automatically to completion. After completion, the run 
  prompt will then be replaced by a *report*, either "a.bbb= OK" or 
  "a.bbb= FAIL", the equals "=" identifies this as report.
- The report is also printed on the Serial console (with more details 
  in case of a fail).
- When the report is shown, either press A to run the test again
  or press X or Y to select another test. Typical is to press Y
  to select the next test, and then A to execute the next test.
- The first test case is "fake" it clears the stats.
  The last test case is also "fake" it prints the stats, hopefully
  something like exec:16, success:16.
  
- Some tests need human *verification* (eg check a LED). What to check
  will be shown in the run prompt. The test completes with the user 
  pressing Y or X; Y means "Yes, ok", and X means "No, not ok". 
  This kind of test has a run prompt of the form 
  "a.bbb> dddd dddd [XY]", note the [XY] as hint to press X or Y.
- Some test need human *action* (eg press a button). What action the 
  operator has to perform is shown in the run prompt. The test typically 
  completes when the test software has detected the action. This kind 
  of test has a run prompt of the form "a.bbb> dddd dddd [TO]", 
  note the [TO], which stands for Time-Out. 
  If the operator does not complete the action within 30 sec or
  when the hardware is broken and the system can not detect the
  action, the test ends after a timeout with a report of the
  form "a.bbb= TIMEOUT".
*/


void mac_show() {
  uint8_t mac[8];
	if( esp_efuse_mac_get_default(mac)!=ESP_OK ) {
    memset( mac, 0, sizeof(mac) );
  } 
  Serial.printf("mac: %02X:%02X:%02X:%02X %02X:%02X:%02X:%02X\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],mac[6],mac[7]);
}


int test_id;


void setup() {
  // Identify over Serial
  Serial.begin(115200);
  do delay(250); while( ! Serial );
  Serial.printf(EVKTESTER_BANNER);
  Serial.printf("%s - version %s\n\n", EVKTESTER_LONGNAME, EVKTESTER_VERSION);

  // Init OSP libs
  aospi_init();
  aoosp_init();
  aoui32_init();

  // Show end of init
  Serial.printf("\n");
  aoui32_oled_splash(EVKTESTER_LONGNAME,EVKTESTER_VERSION); 
  delay(2000);

  Serial.printf("version: %s\n", EVKTESTER_VERSION);
  Serial.printf("cases: %d\n", cases_count() );
  mac_show();
  Serial.printf("\n");
  test_id= 0;
  cases_run(test_id,0);
}


void loop() {
  aoui32_but_scan();

  // Select a test
  if( aoui32_but_wentdown(AOUI32_BUT_X) ) test_id= (test_id+cases_count()-1) % cases_count(); 
  if( aoui32_but_wentdown(AOUI32_BUT_Y) ) test_id= (test_id+cases_count()+1) % cases_count(); 

  // Execute the selected test
  if( aoui32_but_wentdown(AOUI32_BUT_ALL) ) cases_run( test_id, aoui32_but_wentdown(AOUI32_BUT_A) );
}

