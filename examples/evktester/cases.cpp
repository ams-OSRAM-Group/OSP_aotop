// cases.cpp - testcases for evktester
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
#include <Arduino.h>
#include <aospi.h>
#include <aoosp.h>
#include <aomw.h>
#include <aoui32.h>
#include <cases.h>


// WARNING
// The strings passed in aoui32_oled_msgf() have extraneous spaces
// The make the string look better on the OLED (better word breaks)


// === STATS =================================================================


// Time-out for test cases that wait for a user action on tested hardware.
// If the hardware is not ok, the test can not detect the action, and it would never end.
#define REPORT_TIMEOUT_MS (30*1000) 


// Test cases have these possible outcomes.
typedef enum report_e { report_none, report_running, report_success, report_fail, report_timeout }  report_t;

// Converts report tag to a string.
const char * report_to_str(report_t report) {
  const char * str[] = { "NONE", "BUSY", "OK", "FAIL", "TIMEOUT" };
  return str[report];
}


// Collecting all test results
int stats_exec;
int stats_running;
int stats_success;
int stats_fail;
int stats_timeout;


// "fake test" (at the start of the sequence) to reset the stats
// `tid` is the test test id
// `exec` determines whether the test is run (1), or it shows its description.
report_t cases_start(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.start: reset stats     Press A to start a testThen Y to go to next",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.start> running",tid); 

  // Run test (auto)
  stats_exec= 0; 
  stats_running= 0;
  stats_success= 0;
  stats_fail= 0;
  stats_timeout= 0;
  delay(1000); // so that "running" is readable on OLED

  // Report test result
  aoui32_oled_msgf("%d.start= stats reset",tid );
  Serial.printf("START\n" );
  return report_none;
}


// "fake test" (at the end of the sequence) to print the stats
report_t cases_end(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.end: end of test     session (stats printed)",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.end> running",tid); 

  // Run test (auto)
  Serial.printf( "exec    : %d\n", stats_exec);
  Serial.printf( "running : %d\n", stats_running );
  Serial.printf( "success : %d\n", stats_success );
  Serial.printf( "fail    : %d\n", stats_fail );
  Serial.printf( "timeout : %d\n", stats_timeout );
  delay(1000); // so that "running" is readable on OLED

  // Report test result
  aoui32_oled_msgf("%d.end= stats printed - press RST now to test");
  Serial.printf("END\n\n" );
  return report_none;
}


// === HELPER ================================================================


// Macro to return from the helper function with a report_fail (and print detailed msg... to Serial)
#define RETURNFAIL(msg...) do { Serial.printf(msg); return report_fail; } while(0)
// Macro to check osp telegram result code, and if not ok, RETURNFAIL
#define CHECKRESULT(msg...) do { if(result!=aoresult_ok) RETURNFAIL(msg); } while(0)


// PWM test level
#define PWMLEVEL 0x0FFF

// Expected topology, "nr 1", for most test cases
static const char testtopo1_ids[]="_SSSRSRRRSSSRRRRRRRRRRRRRRRRRRRRS"; // SAID (S) resp RGBI (R) in chain; _ is dummy for addr 000
#define TESTTOPO1_OSP32_SAIDOUT_ADDR      0x001
#define TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR  0x005


// Helper for the cases_topo - it checks if the actual topology is indeed OSP32-SAIDbasic-SAIDlooker-CAN-CAN-RGBIstrip-terminator
static report_t run_testtopo1(bool bidir) {
  aoresult_t result;
  int last = bidir ? 31 : 32; // add OSP32.SAIDIN when in loop

  // (1) check chain length
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));
  if( aoosp_exec_resetinit_last()!=last ) RETURNFAIL("chain len is %d, expected %d\n",aoosp_exec_resetinit_last(),last);
  if( bidir ) {
    if( !aospi_dirmux_is_bidir() ) RETURNFAIL("chain runs in loop, expected bidir\n");
  } else {
    if( !aospi_dirmux_is_loop() ) RETURNFAIL("chain runs in bidir, expected loop\n");
  }
  
  // (2) check IDENTIFY against testtopo1_ids
  for( uint16_t addr=0x001; addr<=last; addr++ ) {
    uint32_t id;
    result= aoosp_send_identify(addr, &id ); CHECKRESULT("identify(%03X) %d/%s\n",addr,result,aoresult_to_str(result));
    if( testtopo1_ids[addr]=='S' && !AOOSP_IDENTIFY_IS_SAID(id) ) RETURNFAIL("node %03X must be SAID but has type %08lX\n",addr,id);
    if( testtopo1_ids[addr]=='R' && !AOOSP_IDENTIFY_IS_RGBI(id) ) RETURNFAIL("node %03X must be RGBI but has type %08lX\n",addr,id);
  }

  // (3) check COMST of all SIOs
  for( uint16_t addr=0x001; addr<=last; addr++ ) {
    uint8_t com;
    result= aoosp_send_readcomst( addr, &com ); CHECKRESULT("readcomst(%03X) %d/%s\n",addr,result,aoresult_to_str(result));
    // Check that we have the prescribed chain
    if( addr==0x001 ) {
      if( (com & AOOSP_COMST_SIO1_MASK) != AOOSP_COMST_SIO1_MCU  ) RETURNFAIL("node %03X.SIO1 is not MCU\n",addr);
      if( (com & AOOSP_COMST_SIO2_MASK) != AOOSP_COMST_SIO2_LVDS ) RETURNFAIL("node %03X.SIO2 is not LVDS\n",addr);
    } else if( addr==0x00B ) {
      if( (com & AOOSP_COMST_SIO1_MASK) != AOOSP_COMST_SIO1_LVDS ) RETURNFAIL("node %03X.SIO1 is not LVDS (SAIDlooker plugged via IN?)\n",addr);
      if( (com & AOOSP_COMST_SIO2_MASK) != AOOSP_COMST_SIO2_CAN  ) RETURNFAIL("node %03X.SIO2 is not CAN\n",addr);
    } else if( addr==0x00C ) {
      if( (com & AOOSP_COMST_SIO1_MASK) != AOOSP_COMST_SIO1_CAN  ) RETURNFAIL("node %03X.SIO1 is not CAN (RGBI strip plugged via IN?)\n",addr);
      if( (com & AOOSP_COMST_SIO2_MASK) != AOOSP_COMST_SIO2_LVDS ) RETURNFAIL("node %03X.SIO2 is not LVDS\n",addr);
    } else if( addr==last ) {
      if( (com & AOOSP_COMST_SIO1_MASK) != AOOSP_COMST_SIO1_LVDS ) RETURNFAIL("node %03X.SIO1 is not LVDS\n",addr);
      if( (com & AOOSP_COMST_SIO2_MASK) != AOOSP_COMST_SIO2_EOL  ) RETURNFAIL("node %03X.SIO2 is not EOL\n",addr);
    } else {
      if( (com & AOOSP_COMST_SIO1_MASK) != AOOSP_COMST_SIO1_LVDS ) RETURNFAIL("node %03X.SIO1 is not LVDS\n",addr);
      if( (com & AOOSP_COMST_SIO2_MASK) != AOOSP_COMST_SIO2_LVDS ) RETURNFAIL("node %03X.SIO2 is not LVDS\n",addr);
    }
  }

  // (4) check I2C enabled
  for( uint16_t addr=0x001; addr<=aoosp_exec_resetinit_last(); addr++ ) {
    if(  testtopo1_ids[addr]=='R' ) continue;
    int enabled;
    result= aoosp_exec_i2cenable_get( addr, &enabled ); CHECKRESULT("i2cenable_get(%03X) %d/%s\n",addr,result,aoresult_to_str(result));
    // Check that we have the prescribed chain
    if( addr==0x001 || addr==TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR ) {
      if( !enabled ) RETURNFAIL("node %03X has no I2C, but expected\n",addr);
    } else {
      if(  enabled ) RETURNFAIL("node %03X has I2C, but not expected\n",addr);
    }
  }

  return report_success;
}


// Expected topology, "nr 2", for new SAIDsense
static const char testtopo2_ids[]="_SSSS"; // SAID (S) resp RGBI (R) in chain; _ is dummy for addr 000
#define TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR  0x003


// Helper for the cases_topo - it checks if the actual topology is indeed OSP32-SAIDbasic-SAIDlooker-CAN-CAN-RGBIstrip-terminator
static report_t run_testtopo2(bool bidir) {
  aoresult_t result;
  int last = bidir ? 3 : 4; // add OSP32.SAIDIN when in loop

  // (1) check chain length
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));
  if( aoosp_exec_resetinit_last()!=last ) RETURNFAIL("chain len is %d, expected %d\n",aoosp_exec_resetinit_last(),last);
  if( bidir ) {
    if( !aospi_dirmux_is_bidir() ) RETURNFAIL("chain runs in loop, expected bidir\n");
  } else {
    if( !aospi_dirmux_is_loop() ) RETURNFAIL("chain runs in bidir, expected loop\n");
  }
  
  // (2) check IDENTIFY against testtopo2_ids
  for( uint16_t addr=0x001; addr<=last; addr++ ) {
    uint32_t id;
    result= aoosp_send_identify(addr, &id ); CHECKRESULT("identify(%03X) %d/%s\n",addr,result,aoresult_to_str(result));
    if( testtopo2_ids[addr]=='S' && !AOOSP_IDENTIFY_IS_SAID(id) ) RETURNFAIL("node %03X must be SAID but has type %08lX\n",addr,id);
    if( testtopo2_ids[addr]=='R' && !AOOSP_IDENTIFY_IS_RGBI(id) ) RETURNFAIL("node %03X must be RGBI but has type %08lX\n",addr,id);
  }

  // (3) check COMST of all SIOs
  for( uint16_t addr=0x001; addr<=last; addr++ ) {
    uint8_t com;
    result= aoosp_send_readcomst( addr, &com ); CHECKRESULT("readcomst(%03X) %d/%s\n",addr,result,aoresult_to_str(result));
    // Check that we have the prescribed chain
    if( addr==0x001 ) {
      if( (com & AOOSP_COMST_SIO1_MASK) != AOOSP_COMST_SIO1_MCU  ) RETURNFAIL("node %03X.SIO1 is not MCU\n",addr);
      if( (com & AOOSP_COMST_SIO2_MASK) != AOOSP_COMST_SIO2_LVDS ) RETURNFAIL("node %03X.SIO2 is not LVDS\n",addr);
    } else if( addr==last ) {
      if( (com & AOOSP_COMST_SIO1_MASK) != AOOSP_COMST_SIO1_LVDS ) RETURNFAIL("node %03X.SIO1 is not LVDS\n",addr);
      if( (com & AOOSP_COMST_SIO2_MASK) != AOOSP_COMST_SIO2_EOL  ) RETURNFAIL("node %03X.SIO2 is not EOL\n",addr);
    } else {
      if( (com & AOOSP_COMST_SIO1_MASK) != AOOSP_COMST_SIO1_LVDS ) RETURNFAIL("node %03X.SIO1 is not LVDS\n",addr);
      if( (com & AOOSP_COMST_SIO2_MASK) != AOOSP_COMST_SIO2_LVDS ) RETURNFAIL("node %03X.SIO2 is not LVDS\n",addr);
    }
  }

  // (4) check I2C enabled
  for( uint16_t addr=0x001; addr<=aoosp_exec_resetinit_last(); addr++ ) {
    if(  testtopo2_ids[addr]=='R' ) continue;
    int enabled;
    result= aoosp_exec_i2cenable_get( addr, &enabled ); CHECKRESULT("i2cenable_get(%03X) %d/%s\n",addr,result,aoresult_to_str(result));
    // Check that we have the prescribed chain
    if( addr==0x001 || addr==TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR ) {
      if( !enabled ) RETURNFAIL("node %03X has no I2C, but expected\n",addr);
    } else {
      if(  enabled ) RETURNFAIL("node %03X has I2C, but not expected\n",addr);
    }
  }

  return report_success;
}


// Helper for the cases_osp32i2c - it checks if the SAID OUT on the OSP32 board has a working EEPROM and INT
static report_t run_osp32i2c() {
  aoresult_t result;

  // (1) init chain
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));
  result= aoosp_exec_i2cpower(TESTTOPO1_OSP32_SAIDOUT_ADDR); CHECKRESULT("i2cpower %d/%s\n",result,aoresult_to_str(result));

  // (2) check EEPROM
  uint8_t buf1[1];
  result= aoosp_exec_i2cread8 (TESTTOPO1_OSP32_SAIDOUT_ADDR, AOMW_EEPROM_DADDR7_OSP32, 0xFF, buf1, sizeof(buf1)); CHECKRESULT("i2cread8(1)%d/%s\n",result,aoresult_to_str(result));
  uint8_t buf2[1] = { uint8_t(~buf1[0]) }; 
  result= aoosp_exec_i2cwrite8(TESTTOPO1_OSP32_SAIDOUT_ADDR, AOMW_EEPROM_DADDR7_OSP32, 0xFF, buf2, sizeof(buf2)); CHECKRESULT("i2cwrite8(1) %d/%s\n",result,aoresult_to_str(result));
  uint8_t buf3[1] = { uint8_t(buf1[0] ^ 0xAA) };
  result= aoosp_exec_i2cread8 (TESTTOPO1_OSP32_SAIDOUT_ADDR, AOMW_EEPROM_DADDR7_OSP32, 0xFF, buf3, sizeof(buf3)); CHECKRESULT("i2cread8(2) %d/%s\n",result,aoresult_to_str(result));
  if( buf2[0]!=buf3[0] ) RETURNFAIL("EEPROM did not retain value\n");
  result= aoosp_exec_i2cwrite8(TESTTOPO1_OSP32_SAIDOUT_ADDR, AOMW_EEPROM_DADDR7_OSP32, 0xFF, buf1, sizeof(buf1)); CHECKRESULT("i2cwrite8(2) %d/%s\n",result,aoresult_to_str(result));
  result= aoosp_exec_i2cread8 (TESTTOPO1_OSP32_SAIDOUT_ADDR, AOMW_EEPROM_DADDR7_OSP32, 0xFF, buf3, sizeof(buf3)); CHECKRESULT("i2cread8(2) %d/%s\n",result,aoresult_to_str(result));
  if( buf1[0]!=buf3[0] ) RETURNFAIL("EEPROM could not be restored\n");

  // (3) check INT
  uint32_t now=millis();
  report_t report= report_running;
  int prvstate=0;
  int curstate=0;
  int INT=0;
  while( report==report_running ) { 
    uint8_t flags;
    uint8_t speed;
    result= aoosp_send_readi2ccfg(TESTTOPO1_OSP32_SAIDOUT_ADDR, &flags, &speed ); CHECKRESULT("readi2ccfg %d/%s\n",result,aoresult_to_str(result));
    prvstate= curstate;
    curstate= flags & AOOSP_I2CCFG_FLAGS_INT;
    if( !prvstate && curstate ) { aoui32_led_toggle(AOUI32_LED_GRN); INT++; }

    if( millis()-now > REPORT_TIMEOUT_MS ) report= report_timeout;
    if( INT>=2 ) report=report_success;

    delay(10); // debounce
  }

  return report_success;
}


// Helper for the cases_white - switches all triplets on, to white (this checks if all LEDs work)
static report_t run_white() {
  aoresult_t result;

  // (1) init chain
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));
  result= aoosp_send_clrerror(0x000); CHECKRESULT("clrerror %d/%s\n",result,aoresult_to_str(result));
  result= aoosp_send_goactive(0x000); CHECKRESULT("goactive %d/%s\n",result,aoresult_to_str(result));

  // (2) RGBIs white
  result= aoosp_send_setpwm(0x000,PWMLEVEL,PWMLEVEL,PWMLEVEL,0b000); CHECKRESULT("setpwm %d/%s\n",result,aoresult_to_str(result));

  // (3) SAIDs 3 channels white
  result= aoosp_send_setpwmchn(0x000,0,PWMLEVEL,PWMLEVEL,PWMLEVEL); CHECKRESULT("setpwmchn %d/%s\n",result,aoresult_to_str(result));
  result= aoosp_send_setpwmchn(0x000,1,PWMLEVEL,PWMLEVEL,PWMLEVEL); CHECKRESULT("setpwmchn %d/%s\n",result,aoresult_to_str(result));
  result= aoosp_send_setpwmchn(0x000,2,PWMLEVEL,PWMLEVEL,PWMLEVEL); CHECKRESULT("setpwmchn %d/%s\n",result,aoresult_to_str(result));

  // (4) Wait for operator report
  report_t report = report_running;
  while( report==report_running ) { 
    aoui32_but_scan(); 
    if( aoui32_but_wentdown(AOUI32_BUT_Y) ) report= report_success;
    if( aoui32_but_wentdown(AOUI32_BUT_X) ) report= report_fail;
  }

  // Cleanup (LEDs off)
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));

  return report;
}


// I2C details for the IOX on the SAIDbasic board
#define AOMW_IOX_REGINVAL       0x00 // Input port register reflects the incoming logic levels of all pins (read)
#define AOMW_IOX_REGOUTVAL      0x01 // The Output port register contains the outgoing logic levels of the pins defined as outputs (read/write)
#define AOMW_IOX_REGINPINV      0x02 // The Polarity inversion register allows polarity inversion of pins defined as inputs (read/write)
#define AOMW_IOX_REGCFGINP      0x03 // The Configuration register configures the direction of the I/O pins. If a bit is 1, the pin is input (read/write)


// Helper for the cases_ioxled - it blinks the indicator LEDs on the SAIDbasic board (connected to the IOX)
static report_t run_ioxled() {
  aoresult_t result;

  // (1) init chain
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));

  // (2) setup driver
  result= aoosp_exec_i2cpower(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR); CHECKRESULT("i2cpower %d/%s\n",result,aoresult_to_str(result));
  result= aomw_iox4b4l_present(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR,AOMW_IOX4B4L_DADDR7_SAIDBASIC); CHECKRESULT("iox4b4l_present(%03X) %d/%s\n",TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_iox4b4l_init(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR, AOMW_IOX4B4L_DADDR7_SAIDBASIC, AOMW_IOX4B4L_PINCFG_SAIDBASIC); CHECKRESULT("iox4b4l_init %d/%s\n",result,aoresult_to_str(result));

  // (3) Wait for operator report
  uint32_t toggle= millis();
  report_t report= report_running;
  uint8_t  led_states = AOMW_IOX4B4L_LEDALL;
  while( report==report_running ) { 

    if( millis()-toggle > 500 ) {
      led_states ^= AOMW_IOX4B4L_LEDALL;
      result = aomw_iox4b4l_led_set(led_states); CHECKRESULT("aomw_iox4b4l_led_set %d/%s\n",result,aoresult_to_str(result));
      toggle= millis();
    }

    aoui32_but_scan(); 
    if( aoui32_but_wentdown(AOUI32_BUT_Y) ) report= report_success;
    if( aoui32_but_wentdown(AOUI32_BUT_X) ) report= report_fail;
  }

  // (4) Cleanup (LEDs off)
  result = aomw_iox4b4l_led_off(AOMW_IOX4B4L_LEDALL); CHECKRESULT("aomw_iox4b4l_led_off %d/%s\n",result,aoresult_to_str(result)); CHECKRESULT("i2cwrite8(2) %d/%s\n",result,aoresult_to_str(result));

  return report;
}


// Helper for the cases_ioxbut - checks if all 4 buttons to the IOX are operational
static report_t run_ioxbut() {
  aoresult_t result;

  // (1) init chain
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));

  // (2) setup driver
  result= aoosp_exec_i2cpower(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR); CHECKRESULT("i2cpower %d/%s\n",result,aoresult_to_str(result));
  result= aomw_iox4b4l_present(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR,AOMW_IOX4B4L_DADDR7_SAIDBASIC); CHECKRESULT("iox4b4l_present(%03X) %d/%s\n",TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_iox4b4l_init(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR, AOMW_IOX4B4L_DADDR7_SAIDBASIC, AOMW_IOX4B4L_PINCFG_SAIDBASIC); CHECKRESULT("iox4b4l_init %d/%s\n",result,aoresult_to_str(result));

  // (3) The operator must press al buttons twice
  int b0=0;
  int b1=0;
  int b2=0;
  int b3=0;
  uint32_t now=millis();
  report_t report= report_running;
  while( report==report_running ) { 
    // scan keys
    result= aomw_iox4b4l_but_scan(); CHECKRESULT("aomw_iox4b4l_but_scan %d/%s\n",result,aoresult_to_str(result));
    // test keys
    uint8_t newled=0;
    if( aomw_iox4b4l_but_wentdown(AOMW_IOX4B4L_LED0) ) { b0++; newled|=AOMW_IOX4B4L_LED0; }
    if( aomw_iox4b4l_but_wentdown(AOMW_IOX4B4L_LED1) ) { b1++; newled|=AOMW_IOX4B4L_LED1; }
    if( aomw_iox4b4l_but_wentdown(AOMW_IOX4B4L_LED2) ) { b2++; newled|=AOMW_IOX4B4L_LED2; }
    if( aomw_iox4b4l_but_wentdown(AOMW_IOX4B4L_LED3) ) { b3++; newled|=AOMW_IOX4B4L_LED3; }
    // update indicator led
    if( newled!= 0 ) {
      result = aomw_iox4b4l_led_tgl(newled); CHECKRESULT("aomw_iox4b4l_led_tgl(2) %d/%s\n",result,aoresult_to_str(result));
    }
    // end of test?
    if( millis()-now > REPORT_TIMEOUT_MS ) report= report_timeout;
    if( b0>=2 && b1>=2 && b2>=2 && b3>=2 ) report=report_success;
  }

  // (4) Cleanup (LEDs off)
  result = aomw_iox4b4l_led_off(AOMW_IOX4B4L_LEDALL); CHECKRESULT("aomw_iox4b4l_led_off %d/%s\n",result,aoresult_to_str(result)); CHECKRESULT("i2cwrite8(2) %d/%s\n",result,aoresult_to_str(result));

  return report;
}


report_t run_eeprom_compare(uint8_t daddr7, const uint16_t * script, int count ) {
  aoresult_t result;

  // (1) init chain
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));
  // (2) setup I2C
  result= aoosp_exec_i2cpower(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR); CHECKRESULT("i2cpower %d/%s\n",result,aoresult_to_str(result));
  result= aomw_eeprom_present(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR,daddr7); CHECKRESULT("eeprom_present %d/%s\n",result,aoresult_to_str(result));
  // (3) compare
  result= aomw_eeprom_compare(TESTTOPO1_SAIDBASIC_SAIDI2C_ADDR,daddr7, 0, (const uint8_t*)script, count ); CHECKRESULT("eeprom_compare %d/%s\n",result,aoresult_to_str(result));

  return report_success;
}


// Helper for the cases_senled - it blinks all lights on the SAIDsense board (toplookers, segements, indicators)
static report_t run_senled() {
  aoresult_t result;

  // (1) init chain
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));
  // Prepare for switching on toplookers
  result= aoosp_send_clrerror(0x000); CHECKRESULT("clrerror %d/%s\n",result,aoresult_to_str(result));
  result= aoosp_send_goactive(0x000); CHECKRESULT("goactive %d/%s\n",result,aoresult_to_str(result));

  // (2) setup driver iox4b4l and sseg
  result= aoosp_exec_i2cpower(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("i2c.power %d/%s\n",result,aoresult_to_str(result));
  // selector
  result= aomw_iox4b4l_present(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,AOMW_IOX4B4L_DADDR7_SAIDSENSEV2); CHECKRESULT("iox4b4l_present(%03X) %d/%s\n",TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_iox4b4l_init(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR, AOMW_IOX4B4L_DADDR7_SAIDSENSEV2, AOMW_IOX4B4L_PINCFG_SAIDSENSEV2); CHECKRESULT("iox4b4l_init %d/%s\n",result,aoresult_to_str(result));
  // sseg
  result= aomw_sseg_present(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_sseg_present(%03X) %d/%s\n",TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_sseg_init(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_sseg_init %d/%s\n",result,aoresult_to_str(result));

  // (3) Wait for operator report
  uint32_t toggle= millis();
  report_t report= report_running;
  int mode=0;
  while( report==report_running ) { 

    if( millis()-toggle > 500 ) {
      if( mode==0 ) {
        // indicators on
        result= aoosp_send_setpwmchn(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,0,0,0,0); CHECKRESULT("setpwmchn(0,off) %d/%s\n",result,aoresult_to_str(result));
        result= aoosp_send_setpwmchn(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,1,0,0,0); CHECKRESULT("setpwmchn(1,off) %d/%s\n",result,aoresult_to_str(result));
        result = aomw_iox4b4l_led_on(AOMW_IOX4B4L_LEDALL); CHECKRESULT("aomw_iox4b4l_led_on %d/%s\n",result,aoresult_to_str(result));
        mode=1;
      } else if( mode==1 ) {
        // sseg on
        result = aomw_iox4b4l_led_off(AOMW_IOX4B4L_LEDALL); CHECKRESULT("aomw_iox4b4l_led_of %d/%s\n",result,aoresult_to_str(result));
        result = aomw_sseg_print("8.8.8.8."); CHECKRESULT("aomw_sseg_print %d/%s\n",result,aoresult_to_str(result));
        mode=2;
      } else if( mode==2 ) {
        // toplookers on
        result = aomw_sseg_print("    "); CHECKRESULT("aomw_sseg_print %d/%s\n",result,aoresult_to_str(result));
        result= aoosp_send_setpwmchn(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,0,PWMLEVEL,PWMLEVEL,PWMLEVEL); CHECKRESULT("setpwmchn(0,on) %d/%s\n",result,aoresult_to_str(result));
        result= aoosp_send_setpwmchn(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,1,PWMLEVEL,PWMLEVEL,PWMLEVEL); CHECKRESULT("setpwmchn(1,on) %d/%s\n",result,aoresult_to_str(result));
        mode=0;
      }
      toggle= millis();
    }

    aoui32_but_scan(); 
    if( aoui32_but_wentdown(AOUI32_BUT_Y) ) report= report_success;
    if( aoui32_but_wentdown(AOUI32_BUT_X) ) report= report_fail;
  }

  // (4) Cleanup (LEDs off)
  result = aomw_iox4b4l_led_off(AOMW_IOX4B4L_LEDALL); CHECKRESULT("aomw_iox4b4l_led_off %d/%s\n",result,aoresult_to_str(result)); CHECKRESULT("i2cwrite8(2) %d/%s\n",result,aoresult_to_str(result));

  return report;
}


// Helper for the cases_sensen - checks if all sensors and buttons are operational
static report_t run_sensen() {
  aoresult_t result;

  // (1) init chain
  result= aoosp_exec_resetinit( ); CHECKRESULT("resetinit %d/%s\n",result,aoresult_to_str(result));

  // (2) setup drivers
  result= aoosp_exec_i2cpower(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("i2cpower %d/%s\n",result,aoresult_to_str(result));
  // selector
  result= aomw_iox4b4l_present(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,AOMW_IOX4B4L_DADDR7_SAIDSENSEV2); CHECKRESULT("iox4b4l_present(%03X) %d/%s\n",TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_iox4b4l_init(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR, AOMW_IOX4B4L_DADDR7_SAIDSENSEV2, AOMW_IOX4B4L_PINCFG_SAIDSENSEV2); CHECKRESULT("iox4b4l_init %d/%s\n",result,aoresult_to_str(result));
  // temperature
  result= aomw_as6212_present(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_as6212_present(%03X) %d/%s\n",TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_as6212_init(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_as6212_init %d/%s\n",result,aoresult_to_str(result));
  result= aomw_as6212_convrate_set(125); CHECKRESULT("aomw_as6212_convrate_set %d/%s\n",result,aoresult_to_str(result));
  // rotary
  result= aomw_as5600_present(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_as5600_present(%03X) %d/%s\n",TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_as5600_init(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_as5600_init %d/%s\n",result,aoresult_to_str(result));
  // light
  result= aomw_sfh5721_present(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_sfh5721_present(%03X) %d/%s\n",TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_sfh5721_init(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_sfh5721_init %d/%s\n",result,aoresult_to_str(result));
  // sseg
  result= aomw_sseg_present(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_sseg_present(%03X) %d/%s\n",TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR,result,aoresult_to_str(result));
  result= aomw_sseg_init(TESTTOPO2_SAIDSENSE_SAIDI2C_ADDR); CHECKRESULT("aomw_sseg_init %d/%s\n",result,aoresult_to_str(result));

  // (3) The operator must activate all sensors (inlc buttons)
  // counts
  int b0=0; // but 0 press count
  int b1=0;
  int b2=0;
  int b3=0;
  int st=0; // temp change count
  int sr=0; // rotary change count
  int sl=0; // light change count
  int v; // scratch variable to read sensor
  result= aomw_as6212_temp_get(&v); CHECKRESULT("aomw_as6212_temp_get %d/%s\n",result,aoresult_to_str(result));
  int vt=v; // temp value old
  result= aomw_as5600_angle_get(&v); CHECKRESULT("aomw_as5600_angle_get %d/%s\n",result,aoresult_to_str(result));
  int vr=v; // rotary value old
  result= aomw_sfh5721_als_get(&v); CHECKRESULT("aomw_sfh5721_als_get %d/%s\n",result,aoresult_to_str(result));
  int vl=v; // light value old
  // start
  uint32_t now=millis();
  report_t report= report_running;
  while( report==report_running ) { 
    // scan keys
    result= aomw_iox4b4l_but_scan(); CHECKRESULT("aomw_iox4b4l_but_scan %d/%s\n",result,aoresult_to_str(result));
    // test keys
    uint8_t newled=0;
    if( aomw_iox4b4l_but_wentdown(AOMW_IOX4B4L_LED0) ) { b0++; newled|=AOMW_IOX4B4L_LED0; }
    if( aomw_iox4b4l_but_wentdown(AOMW_IOX4B4L_LED1) ) { b1++; newled|=AOMW_IOX4B4L_LED1; }
    if( aomw_iox4b4l_but_wentdown(AOMW_IOX4B4L_LED2) ) { b2++; newled|=AOMW_IOX4B4L_LED2; }
    if( aomw_iox4b4l_but_wentdown(AOMW_IOX4B4L_LED3) ) { b3++; newled|=AOMW_IOX4B4L_LED3; }
    // update indicator led
    if( newled!= 0 ) {
      result = aomw_iox4b4l_led_tgl(newled); CHECKRESULT("aomw_iox4b4l_led_tgl(2) %d/%s\n",result,aoresult_to_str(result));
    }
    // check sensors for sufficient change
    result= aomw_as6212_temp_get(&v); CHECKRESULT("aomw_as6212_temp_get %d/%s\n",result,aoresult_to_str(result));
    if( abs(v-vt)>100 ) { vt=v; st++; }
    result= aomw_as5600_angle_get(&v); CHECKRESULT("aomw_as5600_angle_get %d/%s\n",result,aoresult_to_str(result));
    if( abs(v-vr)>2000 ) { vr=v; sr++; }
    result= aomw_sfh5721_als_get(&v); CHECKRESULT("aomw_sfh5721_als_get %d/%s\n",result,aoresult_to_str(result));
    if( abs(v-vl)>230 ) { vl=v; sl++; }
    uint8_t sseg[] = { (uint8_t)(st==0?8:(st==1?64:1)), (uint8_t)(sr==0?8:(sr==1?64:1)), (uint8_t)(sl==0?8:(sl==1?64:1)), 0};
    result= aomw_sseg_set(sseg); CHECKRESULT("aomw_sseg_set %d/%s\n",result,aoresult_to_str(result));
    // end of test?
    if( millis()-now > REPORT_TIMEOUT_MS ) report= report_timeout;
    if( b0>=2 && b1>=2 && b2>=2 && b3>=2 && st>=2 && sr>=2 && sl>=2) report=report_success;
  }

  // (4) Cleanup (LEDs off)
  result = aomw_iox4b4l_led_off(AOMW_IOX4B4L_LEDALL); CHECKRESULT("aomw_iox4b4l_led_off %d/%s\n",result,aoresult_to_str(result)); CHECKRESULT("i2cwrite8(2) %d/%s\n",result,aoresult_to_str(result));

  return report;
}


// === CASES =================================================================


report_t cases_uibut(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.uibut: testing the UI   buttons on OSP32",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.uibut> press A, X, Y each twice: OK, ERR, TX toggles [TO]",tid);

  // Run test (operator action)
  int A=0;
  int X=0;
  int Y=0;

  uint32_t now=millis();
  report_t report= report_running;
  while( report==report_running ) { 
    aoui32_but_scan(); 
    if( aoui32_but_wentdown(AOUI32_BUT_A) ) { aoui32_led_toggle(AOUI32_LED_GRN); A++; }
    if( aoui32_but_wentdown(AOUI32_BUT_X) ) { aoui32_led_toggle(AOUI32_LED_RED); X++; }
    if( aoui32_but_wentdown(AOUI32_BUT_Y) ) { aospi_outoena_set( !aospi_outoena_get() ); Y++; }

    if( millis()-now > REPORT_TIMEOUT_MS ) report= report_timeout;
    if( A>=2 && X>=2 && Y>=2 ) report=report_success;
  }

  // Cleanup
  aoui32_led_off(AOUI32_LED_ALL);
  aospi_outoena_set( 0 );

  // Report test result
  aoui32_oled_msgf("%d.uibut= %s",tid, report_to_str(report) );
  Serial.printf("%d.uibut= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_uiled(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.uiled: testing the UI   LEDs on OSP32",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.uiled> check BIDIR, LOOP, OK, ERR, OUT, IN are toggling [XY]", tid);

  // Run test (operator feedback)
  uint32_t toggle= millis();
  report_t report= report_running;
  while( report==report_running ) { 

    if( millis()-toggle > 500 ) {
      aoui32_led_toggle(AOUI32_LED_ALL);
      if( aospi_dirmux_is_bidir() ) aospi_dirmux_set_loop(); else aospi_dirmux_set_bidir();
      aospi_outoena_set( !aospi_outoena_get() );
      aospi_inoena_set( !aospi_inoena_get() );
      toggle= millis();
    }

    aoui32_but_scan(); 
    if( aoui32_but_wentdown(AOUI32_BUT_Y) ) report= report_success;
    if( aoui32_but_wentdown(AOUI32_BUT_X) ) report= report_fail;
  }

  // Cleanup
  aoui32_led_off(AOUI32_LED_ALL);
  aospi_outoena_set( 0 );
  aospi_inoena_set( 0 );

  // Report test result
  aoui32_oled_msgf("%d.uiled= %s",tid, report_to_str(report) );
  Serial.printf("%d.uiled= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_usbpow(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.usbpow: testing 5V   over USB power",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.usbpow> plug 2nd USB in PWR, check LED 4V7 switches to 5V [XY]", tid);

  // Run test (operator feedback)
  report_t report= report_running;
  while( report==report_running ) { 
    aoui32_but_scan(); 
    if( aoui32_but_wentdown(AOUI32_BUT_Y) ) report= report_success;
    if( aoui32_but_wentdown(AOUI32_BUT_X) ) report= report_fail;
  }

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.usbpow= %s",tid, report_to_str(report) );
  Serial.printf("%d.usbpow= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_topo1(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.topo1: join OSP32 SAIDbasic SAIDlooker 2xcan RGBIstrip terminator",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.topo1> running",tid); 

  // Run test (auto)
  report_t report= run_testtopo1(true); // bidir
  delay(1000); // so that "running" is readable on OLED

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.topo1= %s",tid, report_to_str(report) );
  Serial.printf("%d.topo1= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_term2(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.term2: replace termina-tor by second one",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.term2> running",tid); 

  // Run test (auto)
  report_t report= run_testtopo1(true); // bidir
  delay(1000); // so that "running" is readable on OLED

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.term2= %s",tid, report_to_str(report) );
  Serial.printf("%d.term2= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_loop(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.loop:replace terminator by loop-back cable",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.loop> running",tid); 

  // Run test (auto)
  report_t report= run_testtopo1(false); // loop
  delay(1000); // so that "running" is readable on OLED

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.loop= %s",tid, report_to_str(report) );
  Serial.printf("%d.loop= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_white(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.white: check all LEDs (run with USB PWR)",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.white> are all LEDs on and white [XY]",tid); 

  // Run test (auto)
  report_t report= run_white();

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.white= %s",tid, report_to_str(report) );
  Serial.printf("%d.white= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_osp32i2c(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.osp32i2c: test INT and EEPROM on OSP32",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.osp32i2c> press INT twice [TO]",tid); 

  // Run test (auto)
  report_t report= run_osp32i2c();

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.osp32i2c= %s",tid, report_to_str(report) );
  Serial.printf("%d.osp32i2c= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_ioxled(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.ioxled: test indicator   LEDs on SAIDbasic",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.ioxled> are 4 indicator LEDs blinking [XY]",tid); 

  // Run test (auto)
  report_t report= run_ioxled();

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.ioxled= %s",tid, report_to_str(report) );
  Serial.printf("%d.ioxled= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_ioxbut(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.ioxbut: test buttons  on SAIDbasic",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.ioxbut> press all 4 buttons twice [TO]",tid); 

  // Run test (operator action)
  report_t report= run_ioxbut();

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.ioxbut= %s",tid, report_to_str(report) );
  Serial.printf("%d.ioxbut= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_rainbow(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.rainbow: is SAIDbasic EEPROM flashed with   rainbow",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.rainbow> running",tid); 

  // Run test (auto)
  report_t report= run_eeprom_compare(AOMW_EEPROM_DADDR7_SAIDBASIC, aomw_tscript_rainbow(),aomw_tscript_rainbow_bytes());
  delay(1000); // so that "running" is readable on OLED

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.rainbow= %s",tid, report_to_str(report) );
  Serial.printf("%d.rainbow= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_bouncingblock(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.bouncingblock: insert bouncingblock I2C EEPROM stick in SAIDbasic",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.bouncingblock> running",tid); 

  // Run test (auto)
  report_t report= run_eeprom_compare(AOMW_EEPROM_DADDR7_STICK, aomw_tscript_bouncingblock(),aomw_tscript_bouncingblock_bytes());
  delay(1000); // so that "running" is readable on OLED

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.bouncingblock= %s",tid, report_to_str(report) );
  Serial.printf("%d.bouncingblock= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_colormix(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.colormix: insert color-mix I2C EEPROM stick in SAIDbasic",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.colormix> running",tid); 

  // Run test (auto)
  report_t report= run_eeprom_compare(AOMW_EEPROM_DADDR7_STICK, aomw_tscript_colormix(),aomw_tscript_colormix_bytes());
  delay(1000); // so that "running" is readable on OLED

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.colormix= %s",tid, report_to_str(report) );
  Serial.printf("%d.colormix= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_topo2(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.topo2: join OSP32    SAIDsense terminator",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.topo2> running",tid); 

  // Run test (auto)
  report_t report= run_testtopo2(true); // bidir
  delay(1000); // so that "running" is readable on OLED

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.topo2= %s",tid, report_to_str(report) );
  Serial.printf("%d.topo2= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_senled(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.senled:check SAIDsense lights (4 indicator, 32 segment, 2 toplooker)",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.senled> are all lights blinking [XY]",tid); 

  // Run test (operator feedback)
  report_t report= run_senled();

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.senled= %s",tid, report_to_str(report) );
  Serial.printf("%d.senled= %s\n", tid, report_to_str(report) );
  return report;
}


report_t cases_sensen(int tid, int exec) {
  // Print test description
  if( !exec ) { aoui32_oled_msgf("%d.sensen: test sensors SAIDsense",tid); return report_none; }

  // Print test instruction
  aoui32_oled_msgf("%d.ioxbut> trigger all buttons and sensors twice [TO]",tid); 

  // Run test (operator action)
  report_t report= run_sensen();

  // Cleanup

  // Report test result
  aoui32_oled_msgf("%d.ioxbut= %s",tid, report_to_str(report) );
  Serial.printf("%d.ioxbut= %s\n", tid, report_to_str(report) );
  return report;
}



// === RUNNER ================================================================


typedef report_t (*cases_func_t)(int tid, int exec);


cases_func_t cases_funcs[] = {
  /* 0*/ cases_start,
  /* 1*/ cases_uibut,
  /* 2*/ cases_uiled,
  /* 3*/ cases_usbpow,
  /* 4*/ cases_topo1,
  /* 5*/ cases_term2,
  /* 6*/ cases_loop,
  /* 7*/ cases_white,
  /* 8*/ cases_osp32i2c,
  /* 9*/ cases_ioxled,
  /*10*/ cases_ioxbut,
  /*11*/ cases_rainbow,  
  /*12*/ cases_bouncingblock,
  /*13*/ cases_colormix,
  /*14*/ cases_topo2,
  /*15*/ cases_senled,
  /*16*/ cases_sensen,
  /*17*/ cases_end,
};
#define TEST_COUNT (sizeof(cases_funcs)/sizeof(cases_funcs[0]))


int cases_count() {
  return TEST_COUNT;
}


void cases_run(int tid, int exec ) {
  if( tid<0 || tid>= TEST_COUNT ) {
    Serial.printf("ERROR: illegal test id");
    return;
  }
  report_t r= cases_funcs[tid](tid,exec);
  if( r!=report_none ) {
    /*always inc exec's */  stats_exec++;
    if( r==report_running ) stats_running++;
    if( r==report_success ) stats_success++;
    if( r==report_fail    ) stats_fail   ++;
    if( r==report_timeout ) stats_timeout++;
  }
}


